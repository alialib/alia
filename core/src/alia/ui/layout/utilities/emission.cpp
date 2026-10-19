#include <alia/abi/ui/layout/utilities/emission.h>

#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>

using namespace alia;

extern "C" {

void
alia_layout_container_activate(
    alia_ui_context* ctx, alia_layout_container* container)
{
    auto& emission = alia_ctx_layout(ctx)->emission;
    *emission.next_ptr = &container->base;
    emission.next_ptr = &container->first_child;
}

void
alia_layout_container_deactivate(
    alia_ui_context* ctx, alia_layout_container* container)
{
    auto& emission = alia_ctx_layout(ctx)->emission;
    *emission.next_ptr = 0;
    emission.next_ptr = &container->base.next_sibling;
}

struct alia_layout_container_scope
{
    alia_layout_container* container;
};

void
alia_layout_container_simple_begin(
    alia_ui_context* ctx,
    alia_layout_node_vtable* vtable,
    alia_layout_flags_t flags,
    float gap)
{
    if (alia::is_refresh_event(*ctx))
    {
        auto& scope
            = stack_push<alia_layout_container_scope>(alia_ctx_kernel(ctx));
        auto& emission = alia_ctx_layout(ctx)->emission;
        auto* container = arena_alloc<alia_layout_container>(emission.arena);
        scope.container = container;
        *container = alia_layout_container{
            .base = {.vtable = vtable, .next_sibling = 0},
            .flags = flags,
            .first_child = 0,
            .gap = gap};
        alia_layout_container_activate(ctx, container);
    }
}

void
alia_layout_container_simple_end(alia_ui_context* ctx)
{
    if (is_refresh_event(*ctx))
    {
        auto& scope
            = stack_pop<alia_layout_container_scope>(alia_ctx_kernel(ctx));
        alia_layout_container_deactivate(ctx, scope.container);
    }
}

} // extern "C"
