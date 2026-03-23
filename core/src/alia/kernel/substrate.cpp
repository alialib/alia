#include <alia/abi/kernel/substrate.h>

#include <alia/abi/context.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/kernel/substrate.h>

#include <algorithm>

namespace alia {

void
substrate_system_init(
    alia_substrate_system& system, alia_substrate_allocator allocator)
{
    memset(&system, 0, sizeof(system));
    system.allocator = allocator;
}

void
substrate_system_cleanup(alia_substrate_system& system)
{
    substrate_block_cleanup(system, system.root_block);
}

void
substrate_traversal_init(
    alia_substrate_traversal& traversal,
    alia_substrate_system& system,
    alia_bump_allocator* scratch)
{
    memset(&traversal, 0, sizeof(traversal));
    traversal.system = &system;
    traversal.scratch = *scratch;
}

void
substrate_block_destructor(void* ptr)
{
    alia_substrate_block* block = static_cast<alia_substrate_block*>(ptr);
    substrate_block_cleanup(*block->system, *block);
}

void
substrate_block_cleanup(
    alia_substrate_system& system, alia_substrate_block& block)
{
    for (alia_substrate_destructor_record* d = block.destructors; d != nullptr;
         d = d->next)
    {
        d->destructor(d->ptr);
    }
    block.destructors = nullptr;

    if (block.storage != nullptr)
    {
        system.allocator.free(system.allocator.user_data, block.storage);
        block.storage = nullptr;
        ++system.current_generation;
    }
}

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

alia_substrate_usage_result
alia_substrate_use_memory(alia_context* ctx, size_t size, size_t alignment)
{
    auto& traversal = *ctx->substrate;
    switch (traversal.block.mode)
    {
        case ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY: {
            traversal.block.spec.size
                = align_offset(traversal.block.spec.size, alignment) + size;
            traversal.block.spec.align
                = std::max(alignment, traversal.block.spec.align);
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
alia_substrate_use_object(
    alia_context* ctx,
    size_t size,
    size_t alignment,
    void (*destructor)(void*))
{
    // 'Use' the memory for the object.
    alia_substrate_usage_result mr
        = alia_substrate_use_memory(ctx, size, alignment);
    // 'Use' the memory for the destructor record.
    alia_substrate_usage_result dr = alia_substrate_use_memory(
        ctx,
        sizeof(alia_substrate_destructor_record),
        alignof(alia_substrate_destructor_record));
    // If the destructor record is new, initialize it and add it to the block's
    // destructor list.
    if (dr.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
    {
        auto& traversal = *ctx->substrate;
        alia_substrate_destructor_record* record
            = new (dr.ptr) alia_substrate_destructor_record{
                traversal.block.data->destructors, destructor, mr.ptr};
        traversal.block.data->destructors = record;
    }
    return mr;
}

alia_substrate_block*
alia_substrate_use_block(alia_context* ctx)
{
    auto& traversal = *ctx->substrate;
    alia_substrate_usage_result result = alia_substrate_use_object(
        ctx,
        sizeof(alia_substrate_block),
        alignof(alia_substrate_block),
        substrate_block_destructor);
    if (result.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
    {
        return new (result.ptr) alia_substrate_block{
            .system = traversal.system,
            .storage = nullptr,
            .destructors = nullptr,
            .generation = 0};
    }
    else
    {
        return static_cast<alia_substrate_block*>(result.ptr);
    }
}

void
alia_substrate_reset_block(alia_substrate_block* block)
{
    substrate_block_cleanup(*block->system, *block);
}

// Begin the scope of a child substrate block.
// `spec` is the memoized memory layout specification of the block.
// If `alia_substrate_block_needs_discovery` returns true on the spec, the
// traversal will enter discovery mode for the block.
void
alia_substrate_begin_block(
    alia_context* ctx, alia_substrate_block* block, alia_struct_spec* spec)
{
    auto& parent_scope = stack_push<alia_substrate_block_traversal_state>(ctx);

    auto& traversal = *ctx->substrate;
    parent_scope = traversal.block;
    traversal.block.data = block;
    traversal.block.parent = &parent_scope;
    traversal.block.offset_in_parent = parent_scope.current_offset;
    traversal.block.current_offset = 0;

    if (alia_substrate_block_needs_discovery(spec))
    {
        ALIA_ASSERT(get_event_type(*ctx) == ALIA_EVENT_REFRESH);
        as_refresh_event(*ctx).incomplete = true;
        traversal.block.spec = {.size = 0, .align = 0};
        traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY;
    }
    else
    {
        traversal.block.spec = *spec;
        if (block->storage == nullptr)
        {
            block->storage = reinterpret_cast<std::uint8_t*>(
                traversal.system->allocator.alloc(
                    traversal.system->allocator.user_data,
                    spec->size,
                    spec->align));
            block->generation = traversal.system->current_generation;
            traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT;
        }
        else
        {
            traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;
        }
    }
}

// End the scope of the current substrate block.
// Returns the layout specification of the block. - If the block was in
// discovery mode, this should be used to update the memoized spec value.
alia_struct_spec
alia_substrate_end_block(alia_context* ctx)
{
    auto& parent_scope = stack_pop<alia_substrate_block_traversal_state>(ctx);
    auto& traversal = *ctx->substrate;
    alia_struct_spec const completed_block_spec = traversal.block.spec;
    traversal.block = parent_scope;
    return completed_block_spec;
}

ALIA_EXTERN_C_END
