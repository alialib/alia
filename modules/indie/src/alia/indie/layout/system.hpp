#ifndef ALIA_INDIE_LAYOUT_SYSTEM_HPP
#define ALIA_INDIE_LAYOUT_SYSTEM_HPP

#include <alia/indie/layout/node_interface.hpp>
#include <alia/indie/layout/utilities.hpp>

// This file defines the top-level interface to the layout system.

namespace alia { namespace indie {

// This information is required by the layout system to resolve layout
// specifications that refer to the current style.
struct layout_style_info
{
    vector<2, layout_scalar> padding_size;
    float font_size;
    vector<2, layout_scalar> character_size;
    float x_height;
    float magnification;
};

template<class Container, class Node>
void
initialize_layout_traversal(
    layout_traversal<Container, Node>& traversal,
    Node** root_node,
    bool is_refresh,
    counter_type refresh_counter,
    layout_style_info* style,
    vector<2, float> const& ppi)
{
    traversal.active_container = 0;
    traversal.next_ptr = root_node;
    traversal.is_refresh_pass = is_refresh;
    traversal.refresh_counter = refresh_counter;
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
resolve_layout(Container* root_node, layout_vector const& size)
{
    if (root_node)
    {
        root_node->get_horizontal_requirements();
        layout_requirements y = root_node->get_vertical_requirements(size[0]);
        root_node->set_relative_assignment(relative_layout_assignment{
            layout_box(make_layout_vector(0, 0), size), y.ascent});
    }
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
