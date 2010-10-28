#ifndef ALIA_OPENGL_TEXTURE_HPP
#define ALIA_OPENGL_TEXTURE_HPP

#include <alia/opengl/gl.hpp>
#include <alia/opengl/context.hpp>
#include <vector>

namespace alia { namespace opengl {

class simple_texture : public texture
{
 public:
    static unsigned const minimum_tile_size = 128;

    simple_texture(context* ctx, image_interface const& img, unsigned flags);

    vector2i const& size() const { return image_size_; }

    void replace(image_interface const& img);

    void draw(point2d const& p);
    void draw_region(point2d const& p, box2d const& region);

    ~simple_texture();

 private:
    vector2i image_size_, texture_size_;
    GLuint texture_name_;
    unsigned flags_;
};

}}

#endif
