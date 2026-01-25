#include <alia/layout/container.hpp>

#include <alia/abi/ui/style.h>
#include <alia/context.hpp>
#include <alia/events.hpp>

namespace alia {

void
begin_container(
    context& ctx,
    layout_container_scope& scope,
    layout_node_vtable* vtable,
    layout_flag_set flags)
{
    if (is_refresh_event(ctx))
    {
        auto& layout = as_refresh_event(ctx).layout_emission;
        layout_container* container
            = arena_alloc<layout_container>(*layout.arena);
        scope.container = container;
        *container = layout_container{
            .base = {.vtable = vtable, .next_sibling = 0},
            .flags = flags,
            .first_child = 0};
        *layout.next_ptr = &container->base;
        layout.next_ptr = &container->first_child;
    }
}

void
end_container(context& ctx, layout_container_scope& scope)
{
    if (is_refresh_event(ctx))
    {
        auto& layout = as_refresh_event(ctx).layout_emission;
        *layout.next_ptr = 0;
        layout.next_ptr = &scope.container->base.next_sibling;
    }
}

} // namespace alia
