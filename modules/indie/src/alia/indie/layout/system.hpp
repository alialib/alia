#ifndef ALIA_INDIE_LAYOUT_SYSTEM_HPP
#define ALIA_INDIE_LAYOUT_SYSTEM_HPP

#include <alia/indie/layout/internals.hpp>
#include <alia/indie/layout/utilities.hpp>

// This file defines the top-level interface to the layout system.

namespace alia { namespace indie {

// the persistent state associated with an instance of the layout system
struct persistent_layout_state
{
    counter_type refresh_counter = 1;
};

template<class Container, class Node>
void
initialize_layout_traversal(
    layout_traversal<Container, Node>& traversal,
    persistent_layout_state& persistent_state,
    Node** root_node,
    bool is_refresh,
    layout_style_info* style,
    vector<2, float> const& ppi)
{
    traversal.active_container = 0;
    traversal.next_ptr = root_node;
    traversal.is_refresh_pass = is_refresh;
    traversal.refresh_counter = persistent_state.refresh_counter;
    traversal.style_info = style;
    traversal.ppi = ppi;

    style->font_size = 0;
    style->character_size = make_vector<layout_scalar>(0, 0);
    style->x_height = 0;
    style->padding_size = make_layout_vector(4, 4);
    style->magnification = 1;
}

template<class Container>
void
resolve_layout(
    Container* root_node,
    persistent_layout_state& persistent_state,
    layout_vector const& size)
{
    if (root_node)
    {
        root_node->get_horizontal_requirements();
        layout_requirements y = root_node->get_vertical_requirements(size[0]);
        root_node->set_relative_assignment(relative_layout_assignment{
            layout_box(make_layout_vector(0, 0), size), y.ascent});
    }
    // Increment the refresh counter immediately after resolving layout so
    // that any changes detected after this will be associated with the new
    // counter value and thus cause a recalculation.
    ++persistent_state.refresh_counter;
}

template<class Container>
layout_vector
get_minimum_size(Container* root_node)
{
    if (root_node)
    {
        layout_requirements horizontal
            = root_node->get_horizontal_requirements();
        layout_requirements vertical
            = root_node->get_vertical_requirements(horizontal.size);
        return make_layout_vector(horizontal.size, vertical.size);
    }
    else
    {
        return make_layout_vector(0, 0);
    }
}

}} // namespace alia::indie

#endif
