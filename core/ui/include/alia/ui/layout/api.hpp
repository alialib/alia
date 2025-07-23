#pragma once

#include <utility>

#include <alia/flow/flags.hpp>
#include <alia/ui/geometry.hpp>

namespace alia {

struct Context;
struct LayoutContainer;

// The following flags are used to specify various aspects of a node's layout,
// including how it is placed within the space allocated to it (its alignment).
// Omitting these flags invokes the default behavior for the node, which
// depends on the type of node.

ALIA_DEFINE_FLAG_TYPE(Layout)

// X alignment flags
unsigned const X_ALIGNMENT_BIT_OFFSET = 0;
ALIA_DEFINE_FLAG(Layout, 0b00'000'000'111, X_ALIGNMENT_MASK)
ALIA_DEFINE_FLAG(Layout, 0b00'000'000'001, CENTER_X)
ALIA_DEFINE_FLAG(Layout, 0b00'000'000'010, LEFT)
ALIA_DEFINE_FLAG(Layout, 0b00'000'000'011, RIGHT)
ALIA_DEFINE_FLAG(Layout, 0b00'000'000'100, FILL_X)

// Y alignment flags
unsigned const Y_ALIGNMENT_BIT_OFFSET = 3;
ALIA_DEFINE_FLAG(Layout, 0b00'000'111'000, Y_ALIGNMENT_MASK)
ALIA_DEFINE_FLAG(Layout, 0b00'000'001'000, CENTER_Y)
ALIA_DEFINE_FLAG(Layout, 0b00'000'010'000, TOP)
ALIA_DEFINE_FLAG(Layout, 0b00'000'011'000, BOTTOM)
ALIA_DEFINE_FLAG(Layout, 0b00'000'100'000, FILL_Y)
ALIA_DEFINE_FLAG(Layout, 0b00'000'101'000, BASELINE_Y)

// cross-axis alignment flags
unsigned const CROSS_ALIGNMENT_BIT_OFFSET = 6;
ALIA_DEFINE_FLAG(Layout, 0b00'111'000'000, CROSS_ALIGNMENT_MASK)
ALIA_DEFINE_FLAG(Layout, 0b00'001'000'000, CENTER_CROSS)
ALIA_DEFINE_FLAG(Layout, 0b00'010'000'000, START)
ALIA_DEFINE_FLAG(Layout, 0b00'011'000'000, END)
ALIA_DEFINE_FLAG(Layout, 0b00'100'000'000, FILL_CROSS)
ALIA_DEFINE_FLAG(Layout, 0b00'101'000'000, BASELINE)

// combined alignment flags
LayoutFlagSet const CENTER = CENTER_X | CENTER_Y | CENTER_CROSS;
LayoutFlagSet const FILL = FILL_X | FILL_Y | FILL_CROSS;

// Setting the GROW flag sets the node's growth factor to 1.0.
ALIA_DEFINE_FLAG(Layout, 0b01'000'000'000, GROW)

// For nodes that support padding, the UNPADDED flag disables padding.
ALIA_DEFINE_FLAG(Layout, 0b10'000'000'000, UNPADDED)

struct LayoutContainerScope
{
    LayoutContainer* this_container;
    LayoutContainer* parent_container;
};

void
begin_hbox(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags);

void
end_hbox(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
hbox(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_hbox(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_hbox(ctx, scope);
}

template<class Content>
void
hbox(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    LayoutContainerScope scope;
    begin_hbox(ctx, scope, flags);
    std::forward<Content>(content)();
    end_hbox(ctx, scope);
}

void
begin_vbox(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags);

void
end_vbox(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
vbox(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_vbox(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_vbox(ctx, scope);
}

template<class Content>
void
vbox(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    LayoutContainerScope scope;
    begin_vbox(ctx, scope, flags);
    std::forward<Content>(content)();
    end_vbox(ctx, scope);
}

void
begin_flow(Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags);

void
end_flow(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
flow(Context& ctx, Content&& content)
{
    LayoutContainerScope scope;
    begin_flow(ctx, scope, NO_FLAGS);
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

template<class Content>
void
flow(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    LayoutContainerScope scope;
    begin_flow(ctx, scope, flags);
    std::forward<Content>(content)();
    end_flow(ctx, scope);
}

void
begin_inset(
    Context& ctx,
    LayoutContainerScope& scope,
    Insets insets,
    LayoutFlagSet flags);

void
end_inset(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
inset(Context& ctx, Insets insets, Content&& content)
{
    LayoutContainerScope scope;
    begin_inset(ctx, scope, insets, NO_FLAGS);
    std::forward<Content>(content)();
    end_inset(ctx, scope);
}

} // namespace alia
