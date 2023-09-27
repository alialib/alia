#ifndef ALIA_INDIE_LAYOUT_SYSTEM_HPP
#define ALIA_INDIE_LAYOUT_SYSTEM_HPP

#include <alia/indie/layout/internals.hpp>

// This file defines the top-level interface to the layout system.

// alia's layout system uses an immediate mode interface for specifying the
// contents of the UI. However, such an interface imposes limitations on the
// types of layout algorithms that can be implemented efficiently.
// Thus, alia builds an internal, hierarchical representation of the UI tree
// that is specified by the application, and evaluates its layout by directly
// querying the nodes in that tree. It also uses this representation to track
// changes in the tree and detect when it can reuse cached results.

namespace alia { namespace indie {

void
initialize_layout_traversal(
    layout_system& system,
    layout_traversal& traversal,
    bool is_refresh,
    layout_style_info* style,
    vector<2, float> const& ppi);

// Calculate the minimum space needed by the given layout system.
layout_vector
get_minimum_size(layout_system& system);
// Same, but with arguments broken up for flexibility.
layout_vector
get_minimum_size(layout_node* root_node);

// Given the available space, calculate the proper regions for all nodes in
// the given layout system.
void
resolve_layout(layout_system& system, layout_vector const& size);
// Same, but with arguments broken up for flexibility.
void
resolve_layout(layout_node* root_node, layout_vector const& size);

}} // namespace alia::indie

#endif
