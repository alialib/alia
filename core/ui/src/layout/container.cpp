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
        LayoutContainer* this_container
            = arena_alloc<LayoutContainer>(*layout.arena);
        scope.this_container = this_container;
        *this_container = LayoutContainer{
            .base = {.vtable = vtable, .next_sibling = 0},
            .flags = flags,
            .child_count = 0,
            .first_child = 0};
        *layout.next_ptr = &this_container->base;
        layout.next_ptr = &this_container->first_child;
        scope.parent_container = layout.active_container;
        ++scope.parent_container->child_count;
        layout.active_container = this_container;
    }
}

void
end_container(Context& ctx, LayoutContainerScope& scope)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        *layout.next_ptr = 0;
        layout.next_ptr = &scope.this_container->base.next_sibling;
        layout.active_container = scope.parent_container;
    }
}

} // namespace alia
