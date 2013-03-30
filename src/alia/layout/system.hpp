#ifndef ALIA_LAYOUT_SYSTEM_HPP
#define ALIA_LAYOUT_SYSTEM_HPP

#include <alia/layout/internals.hpp>

// This file defines the top-level interface to the layout system.

// alia's layout system uses an immediate mode interface for specifying the
// contents of the UI. However, such an interface imposes limitations on the
// types of layout algorithms that can be implemented efficiently.
// Thus, alia builds an internal, hierarchical representation of the UI tree
// that is specified by the application, and evaluates its layout by directly
// querying the nodes in that tree. It also uses this representation to track
// changes in the tree and detect when it can reuse cached results.

namespace alia {

// scoped_layout_traversal sets up a non-refresh traversal for a layout system.
struct scoped_layout_traversal
{
    scoped_layout_traversal() {}

    scoped_layout_traversal(
        layout_system& system, layout_traversal& traversal,
        geometry_context& geometry, vector<2,float> const& ppi)
    { begin(system, traversal, geometry, ppi); }

    ~scoped_layout_traversal() { end(); }

    void begin(layout_system& system, layout_traversal& traversal,
        geometry_context& geometry, vector<2,float> const& ppi);

    void end() {}

 private:
    layout_style_info dummy_style_info_;
};

// scoped_layout_refresh sets up a refresh for a layout system.
struct scoped_layout_refresh
{
    scoped_layout_refresh() {}

    scoped_layout_refresh(
        layout_system& system, layout_traversal& traversal,
        vector<2,float> const& ppi)
    { begin(system, traversal, ppi); }

    ~scoped_layout_refresh() { end(); }

    void begin(layout_system& system, layout_traversal& traversal,
        vector<2,float> const& ppi);

    void end() {}

 private:
    layout_style_info dummy_style_info_;
};

// Calculate the minimum space needed by the given layout system.
layout_vector get_minimum_size(layout_system& system);
// Same, but with arguments broken up for flexibility.
layout_vector get_minimum_size(layout_node* root_node, data_graph& cache);

// Given the available space, calculate the proper regions for all nodes in
// the given layout system.
void resolve_layout(layout_system& system, layout_vector const& size);
// Same, but with arguments broken up for flexibility.
void resolve_layout(layout_node* root_node, data_graph& cache,
    layout_vector const& size);

}

#endif
