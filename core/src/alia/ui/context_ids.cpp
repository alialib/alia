#include <alia/abi/ui/context_ids.h>

#include <alia/abi/kernel/substrate.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>

#include <cstring>

namespace {

struct context_id_scope
{
    alia_id_view saved;
};

struct collapse_scope
{
    alia_id_view metrics;
    alia_id_view paint;
};

// persistent record for one collapsed context ID -
// Inline IDs live in `inline_id`. An ID with a tail lives in `slab`, which
// this record owns through the substrate allocator.
struct collapsed_context_id
{
    // published revision - 0 before the first recording begin
    uint64_t revision;
    // captured ID when the whole ID fits in an `alia_captured_id`
    alia_captured_id inline_id;
    // substrate-allocator slab for an ID with a tail
    void* slab;
};

struct collapse_state
{
    collapsed_context_id metrics;
    collapsed_context_id paint;
};

bool
capture_is_inline(alia_struct_spec spec)
{
    return spec.size <= sizeof(alia_captured_id)
        && spec.align <= alignof(alia_captured_id);
}

void
release_collapsed(
    alia_general_allocator* alloc, collapsed_context_id* collapsed)
{
    if (collapsed->slab)
    {
        auto* captured = static_cast<alia_captured_id*>(collapsed->slab);
        alia_struct_spec const spec = alia_captured_id_spec(captured->view);
        alia_captured_id_release(collapsed->slab);
        alloc->free(alloc->user_data, collapsed->slab, spec.size, spec.align);
        collapsed->slab = nullptr;
    }
    else
    {
        alia_captured_id_release(&collapsed->inline_id);
    }
    collapsed->inline_id = {};
}

void
store_collapsed(
    alia_general_allocator* alloc,
    collapsed_context_id* collapsed,
    alia_id_view id)
{
    if (collapsed->revision != 0)
        release_collapsed(alloc, collapsed);
    alia_struct_spec const spec = alia_captured_id_spec(id);
    if (capture_is_inline(spec))
    {
        alia_captured_id_capture_into(
            id, &collapsed->inline_id, sizeof(collapsed->inline_id));
        return;
    }

    collapsed->slab = alloc->alloc(alloc->user_data, spec.size, spec.align);
    ALIA_ASSERT(collapsed->slab);
    alia_captured_id_capture_into(id, collapsed->slab, spec.size);
}

bool
collapsed_matches(collapsed_context_id const* collapsed, alia_id_view incoming)
{
    alia_captured_id const* captured
        = collapsed->slab
            ? static_cast<alia_captured_id const*>(collapsed->slab)
            : &collapsed->inline_id;
    return alia_captured_id_matches_view(captured, incoming);
}

void
consider(
    alia_general_allocator* alloc,
    collapsed_context_id* collapsed,
    alia_id_view incoming)
{
    if (collapsed->revision != 0 && collapsed_matches(collapsed, incoming))
        return;
    store_collapsed(alloc, collapsed, incoming);
    ++collapsed->revision;
}

void
collapse_cleanup(
    alia_substrate_system* system, void* ptr, alia_substrate_cleanup_mode mode)
{
    auto* state = static_cast<collapse_state*>(ptr);
    alia_general_allocator* alloc = alia_substrate_system_allocator(system);
    release_collapsed(alloc, &state->metrics);
    release_collapsed(alloc, &state->paint);
    if (mode == ALIA_SUBSTRATE_CLEAR_CACHE)
        std::memset(state, 0, sizeof(*state));
}

} // namespace

extern "C" {

void
alia_context_metrics_id_push(alia_ui_context* ctx, alia_id_view id)
{
    ALIA_ASSERT(ctx);
    auto& scope = alia::stack_push<context_id_scope>(alia_ctx_kernel(ctx));
    scope.saved = ctx->id.metrics;
    ctx->id.metrics = id;
}

void
alia_context_metrics_id_pop(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    auto& scope = alia::stack_pop<context_id_scope>(alia_ctx_kernel(ctx));
    ctx->id.metrics = scope.saved;
}

void
alia_context_paint_id_push(alia_ui_context* ctx, alia_id_view id)
{
    ALIA_ASSERT(ctx);
    auto& scope = alia::stack_push<context_id_scope>(alia_ctx_kernel(ctx));
    scope.saved = ctx->id.paint;
    ctx->id.paint = id;
}

void
alia_context_paint_id_pop(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    auto& scope = alia::stack_pop<context_id_scope>(alia_ctx_kernel(ctx));
    ctx->id.paint = scope.saved;
}

void
alia_context_id_collapse_begin(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(alia_ctx_substrate(alia_ctx_kernel(ctx)));
    auto& scope = alia::stack_push<collapse_scope>(alia_ctx_kernel(ctx));
    scope.metrics = ctx->id.metrics;
    scope.paint = ctx->id.paint;

    alia_substrate_usage_result usage = alia_substrate_use_object(
        alia_ctx_kernel(ctx),
        sizeof(collapse_state),
        alignof(collapse_state),
        collapse_cleanup);
    auto* state = static_cast<collapse_state*>(usage.ptr);
    if (usage.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
        std::memset(state, 0, sizeof(*state));

    if (alia::is_refresh_event(*ctx))
    {
        alia_general_allocator* alloc = alia_substrate_system_allocator(
            alia_ctx_substrate_system(alia_ctx_kernel(ctx)));
        consider(alloc, &state->metrics, ctx->id.metrics);
        consider(alloc, &state->paint, ctx->id.paint);
    }
    ctx->id.metrics = alia_id_view_make_u64(state->metrics.revision);
    ctx->id.paint = alia_id_view_make_u64(state->paint.revision);
}

void
alia_context_id_collapse_end(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    auto& scope = alia::stack_pop<collapse_scope>(alia_ctx_kernel(ctx));
    ctx->id.metrics = scope.metrics;
    ctx->id.paint = scope.paint;
}

} // extern "C"
