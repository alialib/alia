#include <alia/opengl/simple_texture.hpp>
#include <alia/opengl/utils.hpp>

namespace alia { namespace opengl {

simple_texture::simple_texture(context* ctx, image_interface const& img,
    unsigned flags)
{
    ctx_ = ctx;
    flags_ = flags;

    GLenum image_format = get_gl_format(img.format, flags);
    unsigned const n_channels = get_channel_count(img.format);
    GLenum internal_format = get_internal_format(flags, image_format);

    assert(img.size[0] > 0 && img.size[1] > 0);
    image_size_ = img.size;

    glGenTextures(1, &texture_name_);
    glBindTexture(GL_TEXTURE_2D, texture_name_);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // If the texture dimensions are not powers of two, or if the image has a
    // step, it can't be sent to OpenGL directly, so copy it into temporary
    // image and send that to OpenGL.
    if (!is_power_of_two(image_size_[0]) || !is_power_of_two(image_size_[1]) ||
        img.step[0] != 1)
    {
        for (int i = 0; i < 2; ++i)
            texture_size_[i] = get_next_power_of_two(image_size_[i]);

        boost::scoped_ptr<uint8> tmp(
            new uint8[product(texture_size_) * n_channels]);
        copy_subimage(tmp.get(), reinterpret_cast<uint8 const*>(img.pixels),
            image_size_[0] * n_channels, image_size_[1],
            texture_size_[0] * n_channels,
            img.step[1] * n_channels, img.step[0] * n_channels,
            n_channels);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, texture_size_[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture_size_[0],
            texture_size_[1], 0, image_format, GL_UNSIGNED_BYTE, tmp.get());
    }
    else
    {
        texture_size_ = image_size_;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, img.step[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format, texture_size_[0],
            texture_size_[1], 0, image_format, GL_UNSIGNED_BYTE, img.pixels);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLenum wrap = (flags & surface::TILED_IMAGE) != 0 ? GL_REPEAT : GL_CLAMP;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    check_errors();
}

void simple_texture::replace(image_interface const& img)
{
    assert(img.size == image_size_);

    GLenum image_format = get_gl_format(img.format, flags_);
    unsigned const n_channels = get_channel_count(img.format);

    glBindTexture(GL_TEXTURE_2D, texture_name_);

    if (image_size_ != texture_size_)
    {
        boost::scoped_ptr<uint8> tmp(
            new uint8[product(texture_size_) * n_channels]);
        copy_subimage(tmp.get(), reinterpret_cast<uint8 const*>(img.pixels),
            image_size_[0] * n_channels, image_size_[1],
            texture_size_[0] * n_channels,
            img.step[1] * n_channels, img.step[0] * n_channels,
            n_channels);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, texture_size_[0]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_size_[0],
            texture_size_[1], image_format, GL_UNSIGNED_BYTE, tmp.get());
    }
    else
    {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, img.step[1]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_size_[0],
            texture_size_[1], image_format, GL_UNSIGNED_BYTE, img.pixels);
    }

    check_errors();
}

void simple_texture::draw(point2d const& p)
{
    glBindTexture(GL_TEXTURE_2D, texture_name_);

    double tx = double(image_size_[0]) / texture_size_[0];
    double ty = double(image_size_[1]) / texture_size_[1];

    double x0 = p[0], x1 = x0 + image_size_[0];
    double y0 = p[1], y1 = y0 + image_size_[1];

    glBegin(GL_QUADS);
    glTexCoord2d(0, 0);
    glVertex2d(x0, y0);
    glTexCoord2d(tx, 0);
    glVertex2d(x1, y0);
    glTexCoord2d(tx, ty);
    glVertex2d(x1, y1);
    glTexCoord2d(0, ty);
    glVertex2d(x0, y1);
    glEnd();
}

void simple_texture::draw_region(point2d const& p, box2d const& region)
{
    glBindTexture(GL_TEXTURE_2D, texture_name_);

    point2d const i0 = region.corner, i1 = get_high_corner(region);

    double const
        tx0 = double(i0[0]) / texture_size_[0],
        tx1 = double(i1[0]) / texture_size_[0],
        ty0 = double(i0[1]) / texture_size_[1],
        ty1 = double(i1[1]) / texture_size_[1];

    double const x0 = p[0], x1 = p[0] + region.size[0],
        y0 = p[1], y1 = p[1] + region.size[1];

    glBegin(GL_QUADS);
    glTexCoord2d(tx0, ty0);
    glVertex2d(x0, y0);
    glTexCoord2d(tx1, ty0);
    glVertex2d(x1, y0);
    glTexCoord2d(tx1, ty1);
    glVertex2d(x1, y1);
    glTexCoord2d(tx0, ty1);
    glVertex2d(x0, y1);
    glEnd();
}

simple_texture::~simple_texture()
{
    if (ctx_)
        ctx_->add_pending_texture_deletion(texture_name_);
}

}}
