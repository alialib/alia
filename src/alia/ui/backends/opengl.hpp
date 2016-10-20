#ifndef ALIA_UI_BACKENDS_OPENGL_HPP
#define ALIA_UI_BACKENDS_OPENGL_HPP

#include <alia/ui/internals.hpp>

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

void check_opengl_errors();

// Given an OpenGL extension list and an individual extension, this checks if
// that extension is in the list.
bool is_opengl_extension_in_list(
    char const* extension_list, char const* extension);

// The OpenGL context allows external code to schedule actions that must be
// run when the context is active.
// (These are typically calls to delete internal OpenGL objects.)
struct opengl_action_interface : noncopyable
{
    virtual ~opengl_action_interface() {}

    virtual void execute() = 0;
};

struct opengl_context_impl;

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

    // Execute any scheduled actions.
    // (This is called by the associated surfaces when they know that the
    // actual rendering context is active.)
    void do_scheduled_actions();

    opengl_context_impl* impl_;
};

// External objects wishing to associate themselves with an OpenGL context
// should hold an opengl_context_ref and use the following interface to
// interact with it.
// This interface exists because it's possible for external objects to outlive
// the context and for the context to be reset during the life of external
// objects.
// Also note that this interface isn't thread-safe.
struct opengl_context_ref
{
    opengl_context_ref() { context_ = 0; }
    opengl_context_ref(opengl_context_ref const& other)
    { acquire(other.context_); }
    ~opengl_context_ref() { release(); }
    opengl_context_ref& operator=(opengl_context_ref const& other)
    { release(); acquire(other.context_); return *this; }

    void reset(opengl_context* context) { release(); acquire(context); }

    // Is the reference up-to-date?
    bool is_current() const;

    // Schedule an action.
    // Ownership of the action is assumed by the context, so it should be
    // specifically allocated for this call.
    // Note that the scheduling will only happen if the reference is up-to-date.
    // If it's outdated, the action will be immediately deleted and discarded.
    void schedule_action(opengl_action_interface* action) const;

 private:
    friend struct opengl_context;

    void acquire(opengl_context* context);
    void release();

    // a weak reference to the context
    opengl_context* context_;
    // the version of the context that this object is associated with
    unsigned version_;
};

// OpenGL-specific image caching flags (see below).
ALIA_DEFINE_FLAG_TYPE(opengl_texture)
// Use GL_REPEAT as the wrap mode.
// (The image will only tile correctly if it is smaller than the card's
// maximum texture size and its dimensions are powers of two.)
ALIA_DEFINE_FLAG(opengl_texture, 1, OPENGL_TILED_TEXTURE)

// An OpenGL surface implements the surface interface for an OpenGL surface.
struct opengl_surface : surface
{
    opengl_surface() : ctx_(0), opacity_(1), active_subsurface_(0) {}

    // Call this when the surface is created to associate it with a context.
    void set_opengl_context(opengl_context& ctx) { ctx_ = &ctx; }

    // Get the associated context.
    opengl_context& context() const { return *ctx_; }

    // Gets the ID of the context associated with this surface.
    id_interface const& context_id() const;

    // Call this at the beginning of each rendering pass to initialize the
    // OpenGL rendering state.
    // 'size' is the size of the surface in pixels.
    void initialize_render_state(vector<2,unsigned> const& size);

    // implementation of the surface interface
    void cache_image(cached_image_ptr& data, image_interface const& img);
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

    void set_active_subsurface(offscreen_subsurface* subsurface);

    offscreen_subsurface*
    get_active_subsurface() { return active_subsurface_; }

    void generate_offscreen_subsurface(
        offscreen_subsurface_ptr& subsurface,
        box<2,unsigned> const& region);

    void set_opacity(float opacity) { opacity_ = opacity; }
    float opacity() const { return opacity_; }

 private:
    opengl_context* ctx_;
    mutable id_pair<value_id<opengl_context*>,value_id<unsigned> > context_id_;
    vector<2,unsigned> size_;
    float opacity_;
    offscreen_subsurface* active_subsurface_;
    box<2,double> clip_region_;
};

// Use OpenGL extensions to disable vsync.
void disable_vsync();

}

#endif
