#include <alia/substrate.hpp>

#include <alia/prelude.hpp>

#include <algorithm>
#include <new>

namespace alia {

void
construct_substrate_system(
    alia_substrate_system& system, alia_substrate_allocator allocator)
{
    memset(&system, 0, sizeof(system));
    system.allocator = allocator;
}

void
destruct_substrate_system(alia_substrate_system& system)
{
    destruct_substrate_block(system, system.root_block);
}

void
substrate_begin_traversal(
    alia_substrate_traversal& traversal,
    alia_substrate_system& system,
    alia_bump_allocator* scratch)
{
    traversal.system = &system;
    traversal.scratch = *scratch;
    traversal.block.data = nullptr;
    traversal.block.parent = nullptr;
    traversal.block.offset_in_parent = 0;
    traversal.block.current_offset = 0;
    traversal.block.discovery = nullptr;
    // TODO: Use stack for scope..
    static alia_substrate_block_scope scope;
    substrate_begin_block(
        traversal, scope, system.root_block, system.root_block_spec);
}

void
substrate_end_traversal(alia_substrate_traversal& traversal)
{
    if (traversal.block.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY)
    {
        traversal.system->root_block_spec
            = substrate_get_block_spec(traversal);
    }
    static alia_substrate_block_scope scope;
    substrate_end_block(traversal, scope);
}

alia_substrate_usage_result
substrate_use_memory(
    alia_substrate_traversal& traversal, size_t size, size_t alignment)
{
    switch (traversal.block.mode)
    {
        case ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY: {
            traversal.block.discovery->offset
                = align_offset(traversal.block.discovery->offset, alignment)
                + size;
            traversal.block.discovery->alignment
                = std::max(alignment, traversal.block.discovery->alignment);
            size_t arena_alignment
                = (std::max) (alignment, size_t(ALIA_MIN_ALIGN));
            size_t arena_size = align_offset(size, arena_alignment);
            return {
                alia_arena_ptr(
                    &traversal.scratch,
                    alia_arena_alloc_aligned(
                        &traversal.scratch, arena_size, arena_alignment)),
                traversal.block.data->generation,
                ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY};
        }
        case ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL:
        case ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT:
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

alia_substrate_usage_result
substrate_use_object(
    alia_substrate_traversal& traversal,
    size_t size,
    size_t alignment,
    void (*destructor)(void*))
{
    // 'Use' the memory for the object.
    alia_substrate_usage_result mr
        = substrate_use_memory(traversal, size, alignment);
    // 'Use' the memory for the destructor record.
    alia_substrate_usage_result dr = substrate_use_memory(
        traversal,
        sizeof(alia_substrate_destructor_record),
        alignof(alia_substrate_destructor_record));
    // If the destructor record is new, then initialize it and add it to
    // the block's destructors list.
    if (dr.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
    {
        alia_substrate_destructor_record* record
            = new (dr.ptr) alia_substrate_destructor_record{
                traversal.block.data->destructors, destructor, mr.ptr};
        traversal.block.data->destructors = record;
    }
    return mr;
}

void
alia_substrate_block_destructor(void* ptr)
{
    alia_substrate_block* block = static_cast<alia_substrate_block*>(ptr);
    destruct_substrate_block(*block->system, *block);
}

alia_substrate_block*
substrate_use_block(alia_substrate_traversal& traversal)
{
    alia_substrate_usage_result result = substrate_use_object(
        traversal,
        sizeof(alia_substrate_block),
        alignof(alia_substrate_block),
        alia_substrate_block_destructor);
    if (result.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
        return new (result.ptr)
            alia_substrate_block{.system = traversal.system};
    else
        return static_cast<alia_substrate_block*>(result.ptr);
}

void
destruct_substrate_block(
    alia_substrate_system& system, alia_substrate_block& block)
{
    for (alia_substrate_destructor_record* d = block.destructors; d != nullptr;
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
    alia_substrate_traversal& traversal,
    alia_substrate_block_scope& scope,
    alia_substrate_block& block,
    alia_substrate_block_spec spec)
{
    scope = traversal.block;
    traversal.block.data = &block;
    traversal.block.parent = &scope;
    traversal.block.offset_in_parent = scope.current_offset;
    traversal.block.current_offset = 0;
    if (needs_discovery(spec))
    {
        alia_substrate_block_discovery_state* discovery = new (alia_arena_ptr(
            &traversal.scratch,
            alia_arena_alloc_aligned(
                &traversal.scratch,
                sizeof(alia_substrate_block_discovery_state),
                alignof(alia_substrate_block_discovery_state))))
            alia_substrate_block_discovery_state;
        traversal.block.discovery = discovery;
        traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY;
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
            traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT;
        }
        else
        {
            traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;
        }
    }
}

alia_substrate_block_spec
substrate_get_block_spec(alia_substrate_traversal& traversal)
{
    assert(traversal.block.discovery != nullptr);
    return {
        traversal.block.discovery->offset,
        traversal.block.discovery->alignment};
}

void
substrate_end_block(
    alia_substrate_traversal& traversal, alia_substrate_block_scope& scope)
{
    traversal.block = scope;
}

} // namespace alia
