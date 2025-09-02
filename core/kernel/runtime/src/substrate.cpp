#include <alia/kernel/substrate.hpp>

#include <alia/kernel/base.hpp>

#include <algorithm>
#include <new>

namespace alia {

void
construct_substrate_system(
    substrate_system& system, substrate_allocator allocator)
{
    system.allocator = allocator;
}

void
destruct_substrate_system(substrate_system& system)
{
    destruct_substrate_block(system, system.root_block);
}

void
substrate_begin_traversal(
    substrate_traversal& traversal,
    substrate_system& system,
    alia_scratch_arena* scratch)
{
    traversal.system = &system;
    traversal.scratch = scratch;
    // TODO: Decide how to handle root block w.r.t. discovery and allocation.
    traversal.block.data = nullptr;
    traversal.block.parent = nullptr;
    traversal.block.offset_in_parent = 0;
    traversal.block.current_offset = 0;
    traversal.block.discovery = nullptr;
}

void
substrate_end_traversal(substrate_traversal& traversal)
{
    // TODO: End root block?
}

substrate_usage_result
substrate_use_memory(
    substrate_traversal& traversal, size_t size, size_t alignment)
{
    switch (traversal.block.mode)
    {
        case substrate_block_traversal_mode::DISCOVERY: {
            traversal.block.discovery->offset
                = align_offset(traversal.block.discovery->offset, alignment)
                + size;
            traversal.block.discovery->alignment
                = std::max(alignment, traversal.block.discovery->alignment);
            return {
                alia_scratch_alloc(traversal.scratch, size, alignment),
                traversal.block.data->generation,
                substrate_block_traversal_mode::DISCOVERY};
        }
        case substrate_block_traversal_mode::NORMAL:
        case substrate_block_traversal_mode::INIT:
        default:
            size_t aligned_offset
                = align_offset(traversal.block.current_offset, alignment);
            traversal.block.current_offset = aligned_offset + size;
            return {
                traversal.block.data->storage + aligned_offset,
                traversal.block.data->generation,
                traversal.block.mode};
    }
}

substrate_usage_result
substrate_use_object(
    substrate_traversal& traversal,
    size_t size,
    size_t alignment,
    void (*destructor)(void*))
{
    // 'Use' the memory for the object.
    substrate_usage_result mr
        = substrate_use_memory(traversal, size, alignment);
    // 'Use' the memory for the destructor record.
    substrate_usage_result dr = substrate_use_memory(
        traversal,
        sizeof(substrate_destructor_record),
        alignof(substrate_destructor_record));
    // If the destructor record is new, then initialize it and add it to
    // the block's destructors list.
    if (dr.mode != substrate_block_traversal_mode::NORMAL)
    {
        substrate_destructor_record* record
            = new (dr.ptr) substrate_destructor_record{
                traversal.block.data->destructors, destructor, mr.ptr};
        traversal.block.data->destructors = record;
    }
    return mr;
}

void
substrate_block_destructor(void* ptr)
{
    substrate_block* block = static_cast<substrate_block*>(ptr);
    destruct_substrate_block(*block->system, *block);
}

substrate_block*
substrate_use_block(substrate_traversal& traversal)
{
    substrate_usage_result result = substrate_use_object(
        traversal,
        sizeof(substrate_block),
        alignof(substrate_block),
        substrate_block_destructor);
    if (result.mode != substrate_block_traversal_mode::NORMAL)
        return new (result.ptr) substrate_block{.system = traversal.system};
    else
        return static_cast<substrate_block*>(result.ptr);
}

void
destruct_substrate_block(substrate_system& system, substrate_block& block)
{
    for (substrate_destructor_record* d = block.destructors; d != nullptr;
         d = d->next)
    {
        d->destructor(d->ptr);
    }
    block.destructors = nullptr;

    system.allocator.free(system.allocator.user_data, block.storage);
    block.storage = nullptr;
    ++system.current_generation;
}

void
substrate_begin_block(
    substrate_traversal& traversal,
    substrate_block_scope& scope,
    substrate_block& block,
    substrate_block_spec spec)
{
    scope = traversal.block;
    traversal.block.data = &block;
    traversal.block.parent = &scope;
    traversal.block.offset_in_parent = scope.current_offset;
    traversal.block.current_offset = 0;
    if (needs_discovery(spec))
    {
        substrate_block_discovery_state* discovery = new (alia_scratch_alloc(
            traversal.scratch,
            sizeof(substrate_block_discovery_state),
            alignof(substrate_block_discovery_state)))
            substrate_block_discovery_state;
        traversal.block.discovery = discovery;
        traversal.block.mode = substrate_block_traversal_mode::DISCOVERY;
    }
    else
    {
        traversal.block.discovery = nullptr;
        if (block.storage == nullptr)
        {
            block.storage = reinterpret_cast<std::uint8_t*>(
                traversal.system->allocator.alloc(
                    traversal.system->allocator.user_data,
                    spec.size,
                    spec.alignment));
            block.generation = traversal.system->current_generation;
            traversal.block.mode = substrate_block_traversal_mode::INIT;
        }
        else
        {
            traversal.block.mode = substrate_block_traversal_mode::NORMAL;
        }
    }
}

substrate_block_spec
substrate_get_block_spec(substrate_traversal& traversal)
{
    assert(traversal.block.discovery != nullptr);
    return {
        traversal.block.discovery->offset,
        traversal.block.discovery->alignment};
}

void
substrate_end_block(
    substrate_traversal& traversal, substrate_block_scope& scope)
{
    traversal.block = scope;
}

} // namespace alia
