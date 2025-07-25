#pragma once

#include <cstdint>

#include <alia/ui/layout/flags.hpp>
#include <alia/ui/layout/node.hpp>

namespace alia {

struct Context;
struct LayoutNodeVtable;

struct LayoutContainer
{
    LayoutNode base;
    LayoutFlagSet flags;
    std::uint32_t child_count;
    LayoutNode* first_child;
};

struct LayoutContainerScope
{
    LayoutContainer* this_container;
    LayoutContainer* parent_container;
};

void
begin_container(
    Context& ctx,
    LayoutContainerScope& scope,
    LayoutNodeVtable* vtable,
    LayoutFlagSet flags);

void
end_container(Context& ctx, LayoutContainerScope& scope);

} // namespace alia
