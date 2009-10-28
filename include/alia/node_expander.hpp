#ifndef ALIA_NODE_EXPANDER_HPP
#define ALIA_NODE_EXPANDER_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <alia/flags.hpp>

namespace alia {

// accepted flags: all control flags

struct node_expander_result : control_result<bool> {};

node_expander_result do_node_expander(
    context& ctx, accessor<bool> const& expanded, flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout, region_id id = auto_id);

struct tristate_node_expander_result : control_result<int> {};

tristate_node_expander_result do_tristate_node_expander(
    context& ctx, accessor<int> const& expanded, flag_set flags = NO_FLAGS,
    layout const& layout_spec = default_layout, region_id id = auto_id);

}

#endif
