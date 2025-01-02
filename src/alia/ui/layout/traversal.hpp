#ifndef ALIA_UI_LAYOUT_TRAVERSAL_HPP
#define ALIA_UI_LAYOUT_TRAVERSAL_HPP

#include <alia/ui/layout/specification.hpp>

namespace alia {

struct geometry_context;
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

    // This is the geometry context that the layout system manipulates during
    // non-refresh passes.
    geometry_context* geometry;

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

} // namespace alia

#endif
