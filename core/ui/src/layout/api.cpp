#include <alia/ui/layout/api.hpp>

#include <alia/ui/context.hpp>
#include <alia/ui/layout/resolution.hpp>

namespace alia {

namespace {

void
begin_container(Context& ctx, LayoutContainerScope& scope, LayoutNodeType type)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.layout_emission;
        LayoutNode* this_node = reinterpret_cast<LayoutNode*>(
            layout.arena->allocate(sizeof(LayoutNode), alignof(LayoutNode)));
        scope.this_node = this_node;
        *this_node = LayoutNode{
            .type = type,
            .next_sibling = 0,
            .size = {0, 0},
            .margin = {0, 0},
            .container = {.first_child = 0, .child_count = 0}};
        *layout.next_ptr = this_node;
        layout.next_ptr = &this_node->container.first_child;
        scope.parent_node = layout.active_container;
        ++scope.parent_node->child_count;
        layout.active_container = &this_node->container;
    }
}

void
end_container(Context& ctx, LayoutContainerScope& scope)
{
    if (ctx.pass.type == PassType::Refresh)
    {
        auto& layout = ctx.pass.layout_emission;
        *layout.next_ptr = 0;
        layout.next_ptr = &scope.this_node->next_sibling;
        layout.active_container = scope.parent_node;
    }
}

} // namespace

void
begin_hbox(Context& ctx, LayoutContainerScope& scope)
{
    begin_container(ctx, scope, LayoutNodeType::HBox);
}

void
end_hbox(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

void
begin_vbox(Context& ctx, LayoutContainerScope& scope)
{
    begin_container(ctx, scope, LayoutNodeType::VBox);
}

void
end_vbox(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

void
begin_flow(Context& ctx, LayoutContainerScope& scope)
{
    begin_container(ctx, scope, LayoutNodeType::Flow);
}

void
end_flow(Context& ctx, LayoutContainerScope& scope)
{
    end_container(ctx, scope);
}

} // namespace alia
