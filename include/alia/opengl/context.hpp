#ifndef ALIA_OPENGL_CONTEXT_HPP
#define ALIA_OPENGL_CONTEXT_HPP

#include <boost/noncopyable.hpp>
#include <alia/opengl/gl.hpp>
#include <alia/surface.hpp>

namespace alia { namespace opengl {

class context;

struct texture : boost::noncopyable
{
    // replace the image with another of the same dimensions
    virtual void replace(image_interface const& img) = 0;

    virtual vector2i const& get_size() const = 0;

    virtual void draw(point2d const& p) = 0;
    virtual void draw_region(point2d const& p, box2d const& region) = 0;

    virtual ~texture() {}

    context* ctx_;
};

class context
{
 public:
    context();
    ~context();

    void invalidate_textures();

    void add_pending_texture_deletion(GLuint name);
    void do_pending_texture_deletions();

    void cache_image(
        cached_image_ptr& data,
        image_interface const& img,
        unsigned flags);

    struct impl_data;
    impl_data* impl_;
};

}}

#endif
