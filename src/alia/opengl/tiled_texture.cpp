#include <alia/opengl/tiled_texture.hpp>
#include <alia/opengl/utils.hpp>

namespace alia { namespace opengl {

tiled_texture::tiled_texture(context* ctx, image_interface const& img,
    vector2i const& tile_size, unsigned flags)
{
    ctx_ = ctx;
    flags_ = flags;

    GLenum image_format = get_gl_format(img.format, flags);
    unsigned const n_channels = get_channel_count(img.format);
    GLenum internal_format = get_internal_format(flags, image_format);

    assert(img.size[0] > 0 && img.size[1] > 0);
    image_size_ = img.size;

    tile_size_ = tile_size;
    n_tiles_ = (image_size_ - vector2i(1, 1)) / tile_size + vector2i(1, 1);

    for (int i = 0; i < 2; ++i)
    {
        last_tile_size_[i] = get_next_power_of_two(image_size_[i] -
            (n_tiles_[i] - 1) * tile_size_[i]);
    }

    texture_names_.resize(product(n_tiles_));
    glGenTextures(product(n_tiles_), &texture_names_[0]);

    for (int i = 0; i < n_tiles_[1]; i++)
    {
        for (int j = 0; j < n_tiles_[0]; j++)
        {
            glBindTexture(GL_TEXTURE_2D,
                texture_names_[i * n_tiles_[0] + j]);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // the region of the image that this tile covers
            point2i corner(tile_size_[0] * j, tile_size_[1] * i);
            vector2i size(
                (j == n_tiles_[0] - 1) ? last_tile_size_[0] : tile_size_[0],
                (i == n_tiles_[1] - 1) ? last_tile_size_[1] : tile_size_[1]);

            uint8 const* tile_ptr = reinterpret_cast<uint8 const*>(img.pixels)
                + (corner[1] * img.step[1] + corner[0]) * n_channels;

            if (size != tile_size_)
            {
                boost::scoped_ptr<uint8> tmp(
                    new uint8[product(tile_size_) * n_channels]);
                copy_subimage(tmp.get(), tile_ptr,
                    size[0] * n_channels, size[1],
                    tile_size_[0] * n_channels,
                    img.step[1] * n_channels, img.step[0] * n_channels,
                    n_channels);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, tile_size_[0]);
                glTexImage2D(GL_TEXTURE_2D, 0, internal_format,
                    tile_size_[0], tile_size_[1], 0, image_format,
                    GL_UNSIGNED_BYTE, tmp.get());
            }
            else
            {
                glPixelStorei(GL_UNPACK_ROW_LENGTH, img.step[1]);
                glTexImage2D(GL_TEXTURE_2D, 0, internal_format,
                    tile_size_[0], tile_size_[1], 0, image_format,
                    GL_UNSIGNED_BYTE, tile_ptr);
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        }
    }

    check_errors();
}

void tiled_texture::replace(image_interface const& img)
{
    assert(img.size == image_size_);

    GLenum image_format = get_gl_format(img.format, flags_);
    unsigned const n_channels = get_channel_count(img.format);

    for (int i = 0; i < n_tiles_[1]; i++)
    {
        for (int j = 0; j < n_tiles_[0]; j++)
        {
            glBindTexture(GL_TEXTURE_2D,
                texture_names_[i * n_tiles_[0] + j]);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // the region of the image that this tile covers
            point2i corner(tile_size_[0] * j, tile_size_[1] * i);
            vector2i size(
                (j == n_tiles_[0] - 1) ? last_tile_size_[0] : tile_size_[0],
                (i == n_tiles_[1] - 1) ? last_tile_size_[1] : tile_size_[1]);

            unsigned n_channels = get_channel_count(img.format);
            uint8 const* tile_ptr = reinterpret_cast<uint8 const*>(img.pixels)
                + (corner[1] * img.step[1] + corner[0]) * n_channels;

            if (size != tile_size_)
            {
                boost::scoped_ptr<uint8> tmp(
                    new uint8[product(tile_size_) * n_channels]);
                copy_subimage(tmp.get(), tile_ptr,
                    size[0] * n_channels, size[1],
                    tile_size_[0] * n_channels,
                    img.step[1] * n_channels, img.step[0] * n_channels,
                    n_channels);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, tile_size_[0]);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tile_size_[0],
                    tile_size_[1], image_format, GL_UNSIGNED_BYTE,
                    tmp.get());
            }
            else
            {
                glPixelStorei(GL_UNPACK_ROW_LENGTH, img.step[1]);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tile_size_[0],
                    tile_size_[1], image_format, GL_UNSIGNED_BYTE,
                    tile_ptr);
            }

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        }
    }

    check_errors();
}

tiled_texture::~tiled_texture()
{
    if (ctx_)
    {
        for (std::vector<GLuint>::iterator i = texture_names_.begin();
            i != texture_names_.end(); ++i)
        {
            ctx_->add_pending_texture_deletion(*i);
        }
    }
}

void tiled_texture::draw(point2d const& p)
{
    for (int i = 0; i < n_tiles_[1]; i++)
    {
        double ty = (i == n_tiles_[1] - 1) ?
            double(last_tile_size_[1]) / tile_size_[1] : 1;
        double y0 = p[1] + i * tile_size_[1];
        double y1 = y0 + tile_size_[1] * ty;

        for (int j = 0; j < n_tiles_[0]; j++)
        {
            double tx = (j == n_tiles_[0] - 1) ?
                double(last_tile_size_[0]) / tile_size_[0] : 1;
            double x0 = p[0] + j * tile_size_[0];
            double x1 = x0 + tile_size_[0] * tx;

            glBindTexture(GL_TEXTURE_2D,
                texture_names_[i * n_tiles_[0] + j]);

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
    }
}

void tiled_texture::draw_region(point2d const& p, box2d const& region)
{
    double py0 = 0, py1 = tile_size_[1];
    for (int i = 0; i < n_tiles_[1]; ++i, py0 = py1, py1 += tile_size_[1])
    {
        double const cpy0 = (std::max)(py0, region.corner[1]);
        double const cpy1 = (std::min)(py1, get_high_corner(region)[1]);

        if (cpy0 >= cpy1)
            continue;

        double const ty0 = (cpy0 - py0) / tile_size_[1];
        double const ty1 = (cpy1 - py0) / tile_size_[1];

        double px0 = 0, px1 = tile_size_[0];
        for (int j = 0; j < n_tiles_[0]; ++j, px0 = px1, px1 += tile_size_[0])
        {
            double const cpx0 = (std::max)(px0, region.corner[0]);
            double const cpx1 = (std::min)(px1, get_high_corner(region)[0]);

            if (cpx0 >= cpx1)
                continue;

            double const tx0 = (cpx0 - px0) / tile_size_[0];
            double const tx1 = (cpx1 - px0) / tile_size_[0];

            double const x0 = p[0] + cpx0 - region.corner[0];
            double const x1 = p[0] + cpx1 - region.corner[0];
            double const y0 = p[1] + cpy0 - region.corner[1];
            double const y1 = p[1] + cpy1 - region.corner[1];

            glBindTexture(GL_TEXTURE_2D, texture_names_[i * n_tiles_[0] + j]);

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
    }
}

}}
