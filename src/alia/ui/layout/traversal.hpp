#ifndef ALIA_UI_LAYOUT_TRAVERSAL_HPP
#define ALIA_UI_LAYOUT_TRAVERSAL_HPP

#include <alia/ui/layout/specification.hpp>

namespace alia {

struct layout_style_info;

// layout_traversal is the state associated with a traversal of the scene
// that the application wants to lay out.
template<class Container, class Node>
struct layout_traversal
{
    Container* active_container;

    Node** next_ptr;

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

} // namespace alia

#endif
