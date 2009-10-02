#ifndef ALIA_NODE_EXPANDER_HPP
#define ALIA_NODE_EXPANDER_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>

namespace alia {

// accepted flags: all control flags

struct node_expander_result
{
    bool changed;
    bool new_value;

    // allows use within if statements without other unintended conversions
    typedef bool node_expander_result::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return changed ? &node_expander_result::changed : 0; }
};

node_expander_result do_node_expander(context& ctx,
    accessor<bool> const& expanded, unsigned flags = 0,
    layout const& layout_spec = default_layout, region_id id = auto_id);

}

#endif
