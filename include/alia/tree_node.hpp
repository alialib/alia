#ifndef ALIA_TREE_NODE_HPP
#define ALIA_TREE_NODE_HPP

#include <alia/grid_layout.hpp>
#include <alia/flags.hpp>
#include <alia/node_expander.hpp>

namespace alia {

// accepted flags: INITIALLY_EXPANDED, NO_INDENT

static flag_set const NO_INDENT(CUSTOM_FLAG_0_CODE);

class tree_node : boost::noncopyable
{
 public:
    tree_node() {}
    ~tree_node() { end(); }

    tree_node(
        context& ctx,
        layout const& layout_spec = default_layout,
        flag_set flags = NO_FLAGS,
        accessor<int> const* expanded = 0,
        region_id expander_id = 0)
    { begin(ctx, layout_spec, flags, expanded, expander_id); }

    void begin(
        context& ctx,
        layout const& layout_spec = default_layout,
        flag_set flags = NO_FLAGS,
        accessor<int> const* expanded = 0,
        region_id expander_id = 0);

    void end_header();

    bool do_children();

    tristate_node_expander_result const& get_expander_result() const
    { return expander_result_; }

    void end();

 private:
    context* ctx_;
    flag_set flags_;

    grid_layout full_grid_;
    grid_row top_row_;
    grid_row bottom_row_;
    column_layout column_;
    row_layout label_region_;

    bool active_, do_children_;
    tristate_node_expander_result expander_result_;
};

}

#endif
