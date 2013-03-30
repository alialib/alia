#ifndef ALIA_UI_UTILITIES_RENDERING_HPP
#define ALIA_UI_UTILITIES_RENDERING_HPP

#include <alia/ui/internals.hpp>
#include <alia/ui/utilities/styling.hpp>

// This file provides various utilities for facilitating widget rendering.

namespace alia {

// Some simple context accessor functions that are handy for rendering.

static inline matrix<3,3,double> const&
get_transformation(dataless_ui_context& ctx)
{ return get_geometry_context(ctx).transformation_matrix; }

static inline layout_vector const&
get_padding_size(dataless_ui_context& ctx)
{ return get_layout_traversal(ctx).style_info->padding_size; }

static inline surface&
get_surface(dataless_ui_context& ctx)
{ return *ctx.surface; }

static inline bool
is_render_pass(dataless_ui_context& ctx)
{ return ctx.event->type == RENDER_EVENT; }

// A caching_renderer is used by widgets to render content to a cached image.
// It accepts an ID for the content and a region in which the content should
// be render. If it detects a change in either the content ID or the size of
// the region, it signals to the caller that the content should be rerendered.
//
// caching_renderer_data is the data that must persist between frames.
// caching_renderer is a temporary stack object, fitting the normal alia
// scoped object pattern.
//
typedef keyed_data<cached_image_ptr> caching_renderer_data;
struct caching_renderer
{
    caching_renderer() : surface_(0) {}

    template<class Context>
    caching_renderer(Context& ctx, caching_renderer_data& data,
        id_interface const& content_id, layout_box const& region)
    { begin(ctx, data, content_id, region); }

    ~caching_renderer() { end(); }

    template<class Context>
    void begin(Context& ctx, caching_renderer_data& data,
        id_interface const& content_id, layout_box const& region)
    {
        begin(data, get_surface(ctx), get_geometry_context(ctx),
            content_id, region);
    }

    void begin(caching_renderer_data& data, surface& surface,
        geometry_context& geometry,
        id_interface const& content_id, layout_box const& region);

    void end() {}

    // Does the content need to be rendered?
    bool needs_rendering() const { return needs_rendering_; }

    // Get access to the cached image.
    cached_image_ptr& image() { return data_->value; }

    // Mark the cached image as valid and up-to-date.
    void mark_valid() { data_->is_valid = true; }

    // Draw the cached image to the surface.
    void draw();

    layout_box const& region() const { return region_; }

 private:
    caching_renderer_data* data_;
    surface* surface_;
    layout_box region_;
    bool needs_rendering_;
};

struct themed_rendering_data
{
    owned_id theme_id;
    dispatch_interface_ptr theme_renderer;
    data_block refresh_block, drawing_block;
};

void clear_rendering_data(themed_rendering_data& data);

template<class Interface, class DefaultImplementation>
void
get_themed_renderer(
    dataless_ui_context& ctx, themed_rendering_data& data,
    Interface const** renderer,
    DefaultImplementation const* default_implementation)
{
    *renderer = data.theme_renderer ?
        static_cast<Interface const*>(data.theme_renderer.get()) :
        default_implementation;
}

}

#endif
