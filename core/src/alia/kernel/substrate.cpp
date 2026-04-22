#include <alia/abi/kernel/substrate.h>

#include <alia/abi/context.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/kernel/substrate.h>

#include <algorithm>

namespace alia {

void
substrate_system_init(
    alia_substrate_system& system, alia_general_allocator allocator)
{
    memset(&system, 0, sizeof(system));
    system.allocator = allocator;
}

void
substrate_system_cleanup(alia_substrate_system& system)
{
    substrate_block_cleanup(system, system.root_anchor.block);
    system.root_anchor.block = nullptr;
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
substrate_anchor_destructor(alia_substrate_system* system, void* ptr)
{
    alia_substrate_anchor* anchor = static_cast<alia_substrate_anchor*>(ptr);
    substrate_block_cleanup(*system, anchor->block);
}

void
substrate_block_invoke_destructors(
    alia_substrate_system* system, alia_substrate_block* block)
{
    for (alia_substrate_destructor_record* d = block->destructors;
         d != nullptr;
         d = d->next)
    {
        d->destructor(system, d->ptr);
    }
}

void
substrate_block_cleanup(
    alia_substrate_system& system, alia_substrate_block* block)
{
    substrate_block_invoke_destructors(&system, block);
    block->destructors = nullptr;

    {
        system.allocator.free(
            system.allocator.user_data,
            block,
            block->spec.size,
            block->spec.align);
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
                traversal.block.block->generation,
                ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY};
        }
        case ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL:
        case ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT:
        default:
            size_t aligned_offset
                = align_offset(traversal.block.current_offset, alignment);
            traversal.block.current_offset = aligned_offset + size;
            return {
                reinterpret_cast<std::uint8_t*>(traversal.block.block)
                    + aligned_offset,
                traversal.block.block->generation,
                traversal.block.mode};
    }
}

static void
alia_substrate_add_destructor(
    alia_context* ctx,
    void* storage,
    void (*destructor)(alia_substrate_system*, void*),
    void* object)
{
    auto& traversal = *ctx->substrate;
    alia_substrate_destructor_record* record
        = new (storage) alia_substrate_destructor_record{
            traversal.block.block->destructors, destructor, object};
    traversal.block.block->destructors = record;
}

alia_substrate_usage_result
alia_substrate_use_object(
    alia_context* ctx,
    size_t size,
    size_t alignment,
    void (*destructor)(alia_substrate_system*, void*))
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
        alia_substrate_add_destructor(ctx, dr.ptr, destructor, mr.ptr);
    return mr;
}

alia_substrate_anchor*
alia_substrate_use_anchor(alia_context* ctx)
{
    // TODO: Combine these two allocations/conditions into one.
    alia_substrate_usage_result mr = alia_substrate_use_memory(
        ctx, sizeof(alia_substrate_anchor), alignof(alia_substrate_anchor));
    if (mr.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
    {
        new (mr.ptr) alia_substrate_anchor{.block = nullptr};
    }
    alia_substrate_usage_result dr = alia_substrate_use_memory(
        ctx,
        sizeof(alia_substrate_destructor_record),
        alignof(alia_substrate_destructor_record));
    if (dr.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        alia_substrate_add_destructor(
            ctx, dr.ptr, substrate_anchor_destructor, mr.ptr);
    }
    return static_cast<alia_substrate_anchor*>(mr.ptr);
}

void
alia_substrate_reset_anchor(alia_context* ctx, alia_substrate_anchor* anchor)
{
    if (anchor->block)
    {
        substrate_block_cleanup(*ctx->substrate->system, anchor->block);
        anchor->block = nullptr;
    }
}

static alia_substrate_block*
initialize_block(alia_context* ctx, void* ptr, alia_struct_spec spec)
{
    return new (ptr) alia_substrate_block{
        .destructors = nullptr,
        .generation = ctx->substrate->system->current_generation,
        .spec = spec};
}

// Begin the scope of a child substrate block.
// `spec` is the memoized memory layout specification of the block.
// If `alia_substrate_block_needs_discovery` returns true on the spec, the
// traversal will enter discovery mode for the block.
void
alia_substrate_begin_block(
    alia_context* ctx, alia_substrate_anchor* anchor, alia_struct_spec* spec)
{
    auto& parent_scope = stack_push<alia_substrate_block_traversal_state>(ctx);

    auto& traversal = *ctx->substrate;
    parent_scope = traversal.block;
    traversal.block.parent = &parent_scope;
    traversal.block.offset_in_parent = parent_scope.current_offset;
    traversal.block.current_offset = sizeof(alia_substrate_block);

    if (alia_substrate_block_needs_discovery(spec))
    {
        ALIA_ASSERT(get_event_type(*ctx) == ALIA_EVENT_REFRESH);
        as_refresh_event(*ctx).incomplete = true;
        traversal.block.block = initialize_block(
            ctx,
            alia_arena_ptr(
                &traversal.scratch,
                alia_arena_alloc_aligned(
                    &traversal.scratch,
                    sizeof(alia_substrate_block),
                    alignof(alia_substrate_block))),
            {0, 0});
        traversal.block.spec
            = {.size = sizeof(alia_substrate_block),
               .align = alignof(alia_substrate_block)};
        traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY;
    }
    else
    {
        traversal.block.spec = *spec;
        if (anchor->block == nullptr)
        {
            anchor->block = initialize_block(
                ctx,
                traversal.system->allocator.alloc(
                    traversal.system->allocator.user_data,
                    spec->size,
                    spec->align),
                *spec);
            traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT;
        }
        else
        {
            traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;
        }
        traversal.block.block = anchor->block;
    }
}

// End the scope of the current substrate block.
// Returns the layout specification of the block. - If the block was in
// discovery mode, this should be used to update the memoized spec value.
alia_struct_spec
alia_substrate_end_block(alia_context* ctx)
{
    if (ctx->substrate->block.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY)
    {
        substrate_block_invoke_destructors(
            ctx->substrate->system, ctx->substrate->block.block);
    }
    auto& parent_scope = stack_pop<alia_substrate_block_traversal_state>(ctx);
    auto& traversal = *ctx->substrate;
    alia_struct_spec const completed_block_spec = traversal.block.spec;
    traversal.block = parent_scope;
    return completed_block_spec;
}

ALIA_EXTERN_C_END
