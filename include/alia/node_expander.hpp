#ifndef ALIA_NODE_EXPANDER_HPP
#define ALIA_NODE_EXPANDER_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>

namespace alia {

// accepted flags: all control flags

struct node_expander_result : control_result<bool> {};

node_expander_result do_node_expander(context& ctx,
    accessor<bool> const& expanded, unsigned flags = 0,
    layout const& layout_spec = default_layout, region_id id = auto_id);

}

#endif
