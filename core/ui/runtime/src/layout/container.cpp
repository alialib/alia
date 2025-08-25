#include <alia/ui/layout/container.hpp>

#include <alia/ui/context.hpp>

namespace alia {

void
begin_container(
    Context& ctx,
    LayoutContainerScope& scope,
    LayoutNodeVtable* vtable,
    LayoutFlagSet flags)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        LayoutContainer* container
            = arena_alloc<LayoutContainer>(*layout.arena);
        scope.container = container;
        *container = LayoutContainer{
            .base = {.vtable = vtable, .next_sibling = 0},
            .flags = flags,
            .first_child = 0};
        *layout.next_ptr = &container->base;
        layout.next_ptr = &container->first_child;
    }
}

void
end_container(Context& ctx, LayoutContainerScope& scope)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        *layout.next_ptr = 0;
        layout.next_ptr = &scope.container->base.next_sibling;
    }
}

} // namespace alia
