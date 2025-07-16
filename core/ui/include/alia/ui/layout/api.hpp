#pragma once

#include <utility>

#include <alia/flow/flags.hpp>

namespace alia {

struct Context;
struct LayoutContainer;

ALIA_DEFINE_FLAG_TYPE(Layout)

struct LayoutContainerScope
{
    LayoutContainer* this_container;
    LayoutContainer* parent_container;
};

void
begin_hbox(Context& ctx, LayoutContainerScope& scope, float growth_factor);

void
end_hbox(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
hbox(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_hbox(ctx, scope, 0);
    std::forward<Content>(content)();
    end_hbox(ctx, scope);
}

template<class Content>
void
hbox(Context& ctx, float growth_factor, Content&& content)
{
    LayoutContainerScope scope;
    begin_hbox(ctx, scope, growth_factor);
    std::forward<Content>(content)();
    end_hbox(ctx, scope);
}

void
begin_vbox(Context& ctx, LayoutContainerScope& scope, float growth_factor);

void
end_vbox(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
vbox(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_vbox(ctx, scope, 0);
    std::forward<Content>(content)();
    end_vbox(ctx, scope);
}

template<class Content>
void
vbox(Context& ctx, float growth_factor, Content&& content)
{
    LayoutContainerScope scope;
    begin_vbox(ctx, scope, growth_factor);
    std::forward<Content>(content)();
    end_vbox(ctx, scope);
}

void
begin_flow(Context& ctx, LayoutContainerScope& scope, float growth_factor);

void
end_flow(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
flow(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_flow(ctx, scope, 0);
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

template<class Content>
void
flow(Context& ctx, float growth_factor, Content&& content)
{
    LayoutContainerScope scope;
    begin_flow(ctx, scope, growth_factor);
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

} // namespace alia
