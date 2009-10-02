#ifndef ALIA_OPENGL_TILED_TEXTURE_HPP
#define ALIA_OPENGL_TILED_TEXTURE_HPP

#include <alia/opengl/gl.hpp>
#include <alia/opengl/context.hpp>
#include <vector>

namespace alia { namespace opengl {

class tiled_texture : public texture
{
 public:
    static unsigned const minimum_tile_size = 128;

    tiled_texture(context* ctx, image_interface const& img, unsigned flags);

    vector2i const& get_size() const { return image_size_; }

    void replace(image_interface const& img);

    void draw(point2d const& p);
    void draw_region(point2d const& p, box2d const& region);

    ~tiled_texture();

 private:
    vector2i image_size_, n_tiles_, tile_size_, last_tile_size_;
    std::vector<GLuint> texture_names_;
    unsigned flags_;
};

}}

#endif
