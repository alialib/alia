#include <alia/ui/layout/api.hpp>

#include <alia/ui/context.hpp>
#include <alia/ui/layout/flow.hpp>
#include <alia/ui/layout/hbox.hpp>
#include <alia/ui/layout/inset.hpp>
#include <alia/ui/layout/resolution.hpp>
#include <alia/ui/layout/vbox.hpp>

namespace alia {

namespace {

void
begin_container(
    Context& ctx,
    LayoutContainerScope& scope,
    LayoutNodeVtable* vtable,
    uint8_t growth_factor = 0)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        LayoutContainer* this_container
            = reinterpret_cast<LayoutContainer*>(layout.arena->allocate(
                sizeof(LayoutContainer), alignof(LayoutContainer)));
        scope.this_container = this_container;
        *this_container = LayoutContainer{
            .base = {.vtable = vtable, .next_sibling = 0},
            .props
            = {.x_alignment = LayoutAlignment::Fill,
               .y_alignment = LayoutAlignment::Baseline,
               .growth_factor = growth_factor},
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

} // namespace

void
begin_hbox(Context& ctx, LayoutContainerScope& scope, float growth_factor)
{
    begin_container(ctx, scope, &hbox_vtable, growth_factor);
}

void
end_hbox(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

void
begin_vbox(Context& ctx, LayoutContainerScope& scope, float growth_factor)
{
    begin_container(ctx, scope, &vbox_vtable, growth_factor);
}

void
end_vbox(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

void
begin_flow(Context& ctx, LayoutContainerScope& scope, float growth_factor)
{
    begin_container(ctx, scope, &flow_vtable, growth_factor);
}

void
end_flow(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

void
begin_inset(Context& ctx, LayoutContainerScope& scope, Insets insets)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        InsetLayoutNode* this_container
            = reinterpret_cast<InsetLayoutNode*>(layout.arena->allocate(
                sizeof(InsetLayoutNode), alignof(InsetLayoutNode)));
        scope.this_container
            = reinterpret_cast<LayoutContainer*>(this_container);
        *this_container = InsetLayoutNode{
            .container
            = {.base = {.vtable = &inset_vtable, .next_sibling = 0},
               .props
               = {.x_alignment = LayoutAlignment::Start,
                  .y_alignment = LayoutAlignment::Start,
                  .growth_factor = 0},
               .child_count = 0,
               .first_child = 0},
            .insets = insets};
        *layout.next_ptr = &this_container->container.base;
        layout.next_ptr = &this_container->container.first_child;
        scope.parent_container = layout.active_container;
        ++scope.parent_container->child_count;
        layout.active_container = scope.this_container;
    }
}

void
end_inset(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

} // namespace alia
