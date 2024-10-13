#ifndef ALIA_UI_LIBRARY_NODE_EXPANDER_HPP
#define ALIA_UI_LIBRARY_NODE_EXPANDER_HPP

#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/layout/specification.hpp>

namespace alia {

void
do_node_expander(
    ui_context ctx,
    duplex<bool> expanded,
    layout const& layout_spec = default_layout);

} // namespace alia

#endif
