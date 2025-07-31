#pragma once

#include <utility>

#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/flags.hpp>

namespace alia {

struct AlignmentOverrideNode
{
    LayoutContainer container;
    LayoutFlagSet flags;
};

void
begin_alignment_override(
    Context& ctx, LayoutContainerScope& scope, LayoutFlagSet flags);

void
end_alignment_override(Context& ctx, LayoutContainerScope& scope);

template<class Content>
void
alignment_override(Context& ctx, LayoutFlagSet flags, Content&& content)
{
    LayoutContainerScope scope;
    begin_alignment_override(ctx, scope, flags);
    std::forward<Content>(content)();
    end_alignment_override(ctx, scope);
}

extern LayoutNodeVtable alignment_override_vtable;

} // namespace alia
