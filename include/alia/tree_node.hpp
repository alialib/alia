#ifndef ALIA_TREE_NODE_HPP
#define ALIA_TREE_NODE_HPP

#include <alia/linear_layout.hpp>
#include <alia/grid_layout.hpp>
#include <alia/flags.hpp>

namespace alia {

// accepted flags: INITIALLY_EXPANDED, NO_INDENT

static unsigned const NO_INDENT = CUSTOM_FLAG_0;

class tree_node : boost::noncopyable
{
 public:
    tree_node() {}
    ~tree_node() { end(); }

    tree_node(context& ctx, unsigned flags = 0,
        layout const& layout_spec = default_layout,
        region_id expander_id = 0)
    { begin(ctx, flags, layout_spec, expander_id); }

    void begin(context& ctx, unsigned flags = 0,
        layout const& layout_spec = default_layout,
        region_id expander_id = 0);

    void end_header();

    bool do_children();

    void end();

 private:
    context* ctx_;
    unsigned flags_;

    grid_layout full_grid_;
    grid_row top_row_;
    grid_row bottom_row_;
    column_layout column_;
    row_layout label_region_;

    bool active_, do_children_;
};

}

#endif
