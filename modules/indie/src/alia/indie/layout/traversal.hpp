#ifndef ALIA_INDIE_LAYOUT_TRAVERSAL_HPP
#define ALIA_INDIE_LAYOUT_TRAVERSAL_HPP

#include <alia/indie/layout/api.hpp>

namespace alia { namespace indie {

struct layout_system;
struct layout_container;
struct layout_node;
struct layout_style_info;

// layout_traversal is the state associated with a traversal of the scene
// that the application wants to lay out.
struct layout_traversal
{
    // This is the layout system that this traversal is traversing.
    layout_system* system;

    layout_container* active_container;

    layout_node** next_ptr;

    // Iff this is true, then this a refresh pass.
    // A refresh pass is used to specify the contents of the layout tree.
    // Other passes utilize the layout information for their own purposes.
    bool is_refresh_pass;

    // This is incremented each refresh pass.
    counter_type refresh_counter;

    // This is required for resolving layout specs that are specified in
    // physical units or characters.
    vector<2, float> ppi;
    layout_style_info const* style_info;
};

template<class Context>
bool
is_refresh_pass(Context& ctx)
{
    return get_layout_traversal(ctx).is_refresh_pass;
}

}} // namespace alia::indie

#endif
