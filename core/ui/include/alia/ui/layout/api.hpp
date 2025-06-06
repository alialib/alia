#pragma once

#include <utility>

namespace alia {

struct Context;
struct LayoutSpec;

using LayoutIndex = std::uint32_t;

struct LayoutScope
{
    LayoutIndex index = 0;
    LayoutSpec* parent = nullptr;
};

void
begin_hbox(Context& ctx, LayoutScope& scope);

void
end_hbox(Context& ctx, LayoutScope& scope);

template<class Content>
void
hbox(Context& ctx, Content&& content)
{
    LayoutScope scope;
    begin_hbox(ctx, scope);
    std::forward<Content>(content)();
    end_hbox(ctx, scope);
}

void
begin_vbox(Context& ctx, LayoutScope& scope);

void
end_vbox(Context& ctx, LayoutScope& scope);

template<class Content>
void
vbox(Context& ctx, Content&& content)
{
    LayoutScope scope;
    begin_vbox(ctx, scope);
    std::forward<Content>(content)();
    end_vbox(ctx, scope);
}

} // namespace alia
