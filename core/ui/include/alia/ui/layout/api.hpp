#pragma once

#include <utility>

#include <alia/flow/flags.hpp>

namespace alia {

struct Context;
struct LayoutContainer;

ALIA_DEFINE_FLAG_TYPE(Layout)

// struct Layout
// {
//     float grow = 0;
// };

struct LayoutContainerScope
{
    LayoutContainer* this_container;
    LayoutContainer* parent_container;
};

void
begin_hbox(Context& ctx, LayoutContainerScope& scope);

void
end_hbox(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
hbox(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_hbox(ctx, scope);
    std::forward<Content>(content)();
    end_hbox(ctx, scope);
}

void
begin_vbox(Context& ctx, LayoutContainerScope& scope);

void
end_vbox(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
vbox(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_vbox(ctx, scope);
    std::forward<Content>(content)();
    end_vbox(ctx, scope);
}

void
begin_flow(Context& ctx, LayoutContainerScope& scope);

void
end_flow(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
flow(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_flow(ctx, scope);
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

} // namespace alia
