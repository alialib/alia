#ifndef ALIA_OPENGL_SURFACE_HPP
#define ALIA_OPENGL_SURFACE_HPP

#include <alia/ui/utilities/rendering.hpp>

// This file provides an OpenGL implementation of alia's UI surface interface.

namespace alia {

// opengl_error is thrown (sometimes) when an error is reported by OpenGL.
// (If errors can be safely ignored, they are.)
struct opengl_error : exception
{
    opengl_error(string const& msg)
      : exception("OpenGL error: " + msg)
    {}
    ~opengl_error() throw() {}
};

// Given an OpenGL extension list and an individual extension, this checks if
// that extension is in the list.
bool is_opengl_extension_in_list(
    char const* extension_list, char const* extension);

// An opengl_context manages the persistent state associated with an OpenGL
// context (e.g., texture names).
// An opengl_context may be associated with more than one opengl_surface if
// they share the actual underlying rendering context.
// Note that this is not thread-safe, so if it is shared, it should only be
// shared by surfaces that operate within the same thread.
struct opengl_context : noncopyable
{
    opengl_context();
    ~opengl_context();

    // In some cases, the actual OpenGL context that's associated with this
    // opengl_context may be destroyed and a new one constructed in its place.
    // When this happens, we need to discard all the persistent state that was
    // associated with the old context and reestablish that state in the new
    // context.
    // Calling reset() will trigger this.
    void reset();

    // OpenGL textures that are associated with this context may be destroyed
    // at any time. However, the actual OpenGL texture deletions must be done
    // while the associated OpenGL context is active. Thus, they are buffered.
    // Calling this actually executes the buffered deletions.
    // (This is done automatically by the surfaces when they render.)
    void do_pending_deletions();

    struct impl_data;
    impl_data* impl_;
};

// OpenGL-specific image caching flags (see below).
struct opengl_texture_flag_tag {};
typedef flag_set<opengl_texture_flag_tag> opengl_texture_flag_set;
// Use GL_REPEAT as the wrap mode.
// (The image will only tile correctly if it is smaller than the card's
// maximum texture size and its dimensions are powers of two.)
ALIA_DEFINE_FLAG_CODE(opengl_texture_flag_tag, 1, OPENGL_TILED_TEXTURE)

// An OpenGL surface implements the surface interface for an OpenGL surface.
struct opengl_surface : surface
{
    opengl_surface() : ctx_(0), layer_z_(0) {}

    // Call this when the surface is created to associate it with a context.
    void set_opengl_context(opengl_context& ctx) { ctx_ = &ctx; }

    // Call this whenever the size of the associated OpenGL window changes.
    // (Or just call it every frame right before rendering.)
    void set_size(vector<2,unsigned> const& size) { size_ = size; }

    // Call this at the beginning of each rendering pass to initialize the
    // OpenGL rendering state.
    void initialize_render_state();

    // implementation of the surface interface
    vector<2,unsigned> size() const { return size_; }
    void set_layer_z(double z);
    void cache_image(cached_image_ptr& data, image_interface const& img);
    void create_cached_content(cached_rendering_content_ptr& data);
    void set_transformation_matrix(matrix<3,3,double> const& m);
    void set_clip_region(box<2,double> const& region);
    void draw_filled_box(rgba8 const& color, box<2,double> const& box);

    // OpenGL has a lot more flexibility in loading textures than the surface
    // interface demands. Some applications may want to manage their textures
    // in an IM style through this interface, so we expose a second interface
    // for caching images with more options.
    void cache_image(
        cached_image_ptr& data,
        image_interface const& img,
        opengl_texture_flag_set flags);

 private:
    opengl_context* ctx_;
    vector<2,unsigned> size_;
    double layer_z_;
};

}

#endif
