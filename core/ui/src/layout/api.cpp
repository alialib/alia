#include <alia/ui/layout/api.hpp>

#include <alia/ui/context.hpp>
#include <alia/ui/layout/resolution.hpp>

namespace alia {

namespace {

void
begin_container(Context& ctx, LayoutScope& scope, LayoutNodeType type)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.layout_emission;
        *layout.next = layout.count;
        scope.index = layout.count;
        LayoutNode* new_node = &layout.nodes[scope.index];
        *new_node = LayoutNode{
            .type = type,
            .size = {0, 0},
            .margin = {0, 0},
            .first_child = 0,
            .next_sibling = 0};
        layout.next = &new_node->first_child;
        ++layout.count;
        scope.parent = layout.active_container;
        ++scope.parent->child_count;
        layout.active_container = new_node;
    }
    else
    {
        scope.index = ctx.pass.layout_consumption.index++;
    }
}

void
end_container(Context& ctx, LayoutScope& scope)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.layout_emission;
        LayoutNode* this_node = &layout.nodes[scope.index];
        *layout.next = 0;
        layout.next = &this_node->next_sibling;
        layout.active_container = scope.parent;
    }
}

} // namespace

void
begin_hbox(Context& ctx, LayoutScope& scope)
{
    begin_container(ctx, scope, LayoutNodeType::HBox);
}

void
end_hbox(Context& ctx, LayoutScope& scope)
{
    end_container(ctx, scope);
}

void
begin_vbox(Context& ctx, LayoutScope& scope)
{
    begin_container(ctx, scope, LayoutNodeType::VBox);
}

void
end_vbox(Context& ctx, LayoutScope& scope)
{
    end_container(ctx, scope);
}

void
begin_flow(Context& ctx, LayoutScope& scope)
{
    begin_container(ctx, scope, LayoutNodeType::Flow);
}

void
end_flow(Context& ctx, LayoutScope& scope)
{
    end_container(ctx, scope);
}

} // namespace alia
