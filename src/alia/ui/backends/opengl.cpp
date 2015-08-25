#include <alia/ui/backends/opengl.hpp>
#include <list>
#include <vector>

#include <GL\glew.h>
#include <GL\wglew.h>

#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/skia.hpp>

namespace alia {

void check_errors()
{
    // Check for an error.
    GLenum err = glGetError();

    // Clear any other errors that have also occurred.
    while (glGetError() != GL_NO_ERROR)
        ;

    // If there's no error, abort.
    if (err == GL_NO_ERROR)
        return;

    // Decode the error.
    const char *s;
    switch (err)
    {
     case GL_INVALID_ENUM:
        s = "GL_INVALID_ENUM";
        break;
     case GL_INVALID_VALUE:
        s = "GL_INVALID_VALUE";
        break;
     case GL_INVALID_OPERATION:
        s = "GL_INVALID_OPERATION";
        break;
     case GL_STACK_OVERFLOW:
        s = "GL_STACK_OVERFLOW";
        break;
     case GL_STACK_UNDERFLOW:
        s = "GL_STACK_UNDERFLOW";
        break;
     case GL_OUT_OF_MEMORY:
        s = "GL_OUT_OF_MEMORY";
        break;
     default:
        s = "GL_UNKNOWN_ERROR";
    }

    throw opengl_error(s);
}

bool is_opengl_extension_in_list(
    char const* extension_list, char const* extension)
{
    char const* start = extension_list;
    if (!start)
        return false;

    for(;;)
    {
        char const* where = strstr(start, extension);
        if (!where)
            break;

        char const* terminator = where + strlen(extension);

        if ((where == start || *(where - 1) == ' ') &&
            (*terminator == ' ' || *terminator == '\0'))
        {
            return true;
        }

        start = terminator;
    }

    return false;
}

static bool is_power_of_two(unsigned n)
{
    return (n & (n - 1)) == 0 && n != 0;
}

static unsigned get_next_power_of_two(unsigned n)
{
    unsigned i = 1;
    while (i < n)
        i <<= 1;
    return i;
}

static GLenum get_gl_format(pixel_format fmt)
{
    switch (fmt)
    {
     case ALPHA:
        return GL_ALPHA;
     case RGB:
        return GL_RGB;
     case RGBA:
        return GL_RGBA;
     case GRAY:
     default:
        return GL_LUMINANCE;
    }
}

static void copy_subimage(
    uint8_t* dst, uint8_t const* src, int width, int height,
    int dst_stride, int src_stride, int src_step, int pixel_size)
{
    if (src_step != pixel_size)
    {
        for (int i = 0; i < height; ++i)
        {
            uint8_t const* s = src;
            uint8_t* d_end = dst + width * pixel_size;
            for (uint8_t* d = dst; d != d_end; d += pixel_size, s += src_step)
            {
                for (int j = 0; j < pixel_size; ++j)
                    *(d + j) = *(s + j);
            }
            src += src_stride;
            dst += dst_stride;
        }
    }
    else
    {
        for (int i = 0; i < height; ++i)
        {
            memcpy(dst, src, width);
            src += src_stride;
            dst += dst_stride;
        }
    }
}

struct opengl_texture;
struct offscreen_buffer;

struct opengl_context::impl_data
{
    bool is_initialized;

    // Is GL_ARB_texture_rectangle supported?
    bool texture_rectangle_supported;

    unsigned max_texture_size;

    std::list<opengl_texture*> associated_textures;
    std::list<GLuint> pending_texture_deletions;

    std::list<offscreen_buffer*> associated_offscreen_buffers;
    std::list<GLuint> pending_framebuffer_deletions;
    std::list<GLuint> pending_renderbuffer_deletions;
    offscreen_buffer* active_offscreen_buffer;

    // This is incremented each time the context is reset.
    // When a texture is created, it is assigned the current version number.
    // Thus, only textures with the current version number are valid.
    unsigned version;

    cached_image_ptr uniform_image;
};

// TEXTURES

struct opengl_texture : cached_image
{
    opengl_texture() : ctx_(0) {}

    bool is_valid() const { return context_version_ == ctx_->impl_->version; }

    virtual void replace(image_interface const& img) = 0;

    // In cases where the OpenGL context is reset without our knowledge, we
    // may actually want to leak the texture handles we have.
    void leak() { ctx_ = 0; }

    ~opengl_texture();

    opengl_context* ctx_;
    unsigned context_version_;

    pixel_format format_;
};

opengl_texture::~opengl_texture()
{
    if (ctx_)
        ctx_->impl_->associated_textures.remove(this);
}

namespace {

struct simple_texture : opengl_texture
{
    static unsigned const minimum_tile_size = 128;

    simple_texture(opengl_context* ctx, image_interface const& img,
        opengl_texture_flag_set flags);

    vector<2,unsigned> size() const { return image_size_; }

    void replace(image_interface const& img);

    void draw(
        surface& surface,
        box<2,double> const& surface_region,
        box<2,double> const& image_region,
        rgba8 const& color);

    ~simple_texture();

 private:
    vector<2,unsigned> image_size_, texture_size_;
    GLenum target_;
    GLuint texture_name_;
    opengl_texture_flag_set flags_;
};

simple_texture::simple_texture(opengl_context* ctx, image_interface const& img,
    opengl_texture_flag_set flags)
{
    ctx_ = ctx;
    context_version_ = ctx->impl_->version;
    format_ = img.format;
    flags_ = flags;

    GLenum image_format = get_gl_format(img.format);
    unsigned const n_channels = get_channel_count(img.format);
    GLenum internal_format = image_format;

    assert(img.size[0] > 0 && img.size[1] > 0);
    image_size_ = img.size;

    target_ =
        (ctx->impl_->texture_rectangle_supported &&
        !(flags & OPENGL_TILED_TEXTURE)) ?
        GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D;

    glGenTextures(1, &texture_name_);
    glBindTexture(target_, texture_name_);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // If GL_ARB_texture_rectangle is not supported and the texture dimensions
    // are not powers of two, it can't be sent to OpenGL directly, so copy it
    // into a temporary array and send that to OpenGL.
    if (!ctx->impl_->texture_rectangle_supported &&
        (!is_power_of_two(image_size_[0]) || !is_power_of_two(image_size_[1])))
    {
        for (int i = 0; i < 2; ++i)
            texture_size_[i] = get_next_power_of_two(image_size_[i]);

        alia__shared_ptr<uint8_t> tmp(
            new uint8_t[texture_size_[0] * texture_size_[1] * n_channels]);
        copy_subimage(
            tmp.get(), reinterpret_cast<uint8_t const*>(img.pixels),
            image_size_[0] * n_channels, image_size_[1],
            texture_size_[0] * n_channels,
            img.stride * n_channels, n_channels,
            n_channels);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, texture_size_[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, internal_format,
            texture_size_[0], texture_size_[1], 0,
            image_format, GL_UNSIGNED_BYTE, tmp.get());
    }
    else
    {
        texture_size_ = image_size_;
        glPixelStorei(GL_UNPACK_ROW_LENGTH, img.stride);
        glTexImage2D(target_, 0, internal_format,
            texture_size_[0], texture_size_[1], 0,
            image_format, GL_UNSIGNED_BYTE, img.pixels);
    }

    glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    check_errors();

    if (flags & OPENGL_TILED_TEXTURE)
    {
        glTexParameteri(target_, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(target_, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else
    {
        glTexParameteri(target_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    // Apparently the above sometimes fails, but we can harmlessly ignore this.
    while (glGetError() != GL_NO_ERROR)
        ;
}

void simple_texture::replace(image_interface const& img)
{
    assert(img.size == image_size_);

    GLenum image_format = get_gl_format(img.format);
    unsigned const n_channels = get_channel_count(img.format);

    glBindTexture(target_, texture_name_);

    if (image_size_ != texture_size_)
    {
        alia__shared_ptr<uint8_t> tmp(
            new uint8_t[texture_size_[0] * texture_size_[1] * n_channels]);
        copy_subimage(tmp.get(), reinterpret_cast<uint8_t const*>(img.pixels),
            image_size_[0] * n_channels, image_size_[1],
            texture_size_[0] * n_channels,
            img.stride * n_channels, n_channels,
            n_channels);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, texture_size_[0]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture_size_[0],
            texture_size_[1], image_format, GL_UNSIGNED_BYTE, tmp.get());
    }
    else
    {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, img.stride);
        glTexSubImage2D(target_, 0, 0, 0, texture_size_[0],
            texture_size_[1], image_format, GL_UNSIGNED_BYTE, img.pixels);
    }

    check_errors();
}

void simple_texture::draw(
    surface& surface,
    box<2,double> const& surface_region,
    box<2,double> const& image_region,
    rgba8 const& color)
{
    glEnable(target_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(target_, texture_name_);

    rgba8 resolved_color = interpolate(rgba8(0, 0, 0, 0), color,
        static_cast<opengl_surface&>(surface).opacity());
    glColor4ub(resolved_color.r, resolved_color.g, resolved_color.b,
        resolved_color.a);

    vector<2,double> const
        i0 = image_region.corner,
        i1 = get_high_corner(image_region);

    double tx0, tx1, ty0, ty1;
    if (target_ == GL_TEXTURE_2D)
    {
        tx0 = i0[0] / texture_size_[0],
        tx1 = i1[0] / texture_size_[0],
        ty0 = i0[1] / texture_size_[1],
        ty1 = i1[1] / texture_size_[1];
    }
    else
    {
        tx0 = i0[0];
        tx1 = i1[0];
        ty0 = i0[1];
        ty1 = i1[1];
    }

    double const
        x0 = surface_region.corner[0],
        x1 = x0 + surface_region.size[0],
        y0 = surface_region.corner[1],
        y1 = y0 + surface_region.size[1];

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

    glDisable(target_);
}

simple_texture::~simple_texture()
{
    if (ctx_)
        ctx_->impl_->pending_texture_deletions.push_back(texture_name_);
}

struct tiled_texture : opengl_texture
{
    static unsigned const minimum_tile_size = 128;

    tiled_texture(opengl_context* ctx, image_interface const& img,
        vector<2,unsigned> const& tile_size, opengl_texture_flag_set flags);

    vector<2,unsigned> size() const { return image_size_; }

    void replace(image_interface const& img);

    void draw(
        surface& surface,
        box<2,double> const& surface_region,
        box<2,double> const& image_region,
        rgba8 const& color);

    ~tiled_texture();

 private:
    vector<2,unsigned> image_size_, tile_size_, n_tiles_, last_tile_size_;
    std::vector<GLuint> texture_names_;
    opengl_texture_flag_set flags_;
    GLenum target_;
};

tiled_texture::tiled_texture(opengl_context* ctx, image_interface const& img,
    vector<2,unsigned> const& tile_size, opengl_texture_flag_set flags)
{
    ctx_ = ctx;
    context_version_ = ctx->impl_->version;
    format_ = img.format;
    flags_ = flags;

    target_ = ctx->impl_->texture_rectangle_supported ?
        GL_TEXTURE_RECTANGLE_ARB : GL_TEXTURE_2D;

    GLenum image_format = get_gl_format(img.format);
    unsigned const n_channels = get_channel_count(img.format);
    GLenum internal_format = image_format;

    assert(img.size[0] > 0 && img.size[1] > 0);
    image_size_ = img.size;

    n_tiles_ =
        (image_size_ - make_vector<unsigned>(1, 1)) / tile_size +
        make_vector<unsigned>(1, 1);
    tile_size_ = tile_size;
    last_tile_size_ = image_size_ -
        (n_tiles_ - make_vector<unsigned>(1, 1)) * tile_size_;

    texture_names_.resize(n_tiles_[0] * n_tiles_[1]);
    glGenTextures(n_tiles_[0] * n_tiles_[1], &texture_names_[0]);

    for (unsigned i = 0; i < n_tiles_[1]; i++)
    {
        for (unsigned j = 0; j < n_tiles_[0]; j++)
        {
            glBindTexture(target_, texture_names_[i * n_tiles_[0] + j]);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // the region of the image that this tile covers
            vector<2,unsigned> corner =
                make_vector(tile_size_[0] * j, tile_size_[1] * i);
            vector<2,unsigned> size = make_vector(
                (j == n_tiles_[0] - 1) ? last_tile_size_[0] : tile_size_[0],
                (i == n_tiles_[1] - 1) ? last_tile_size_[1] : tile_size_[1]);

            uint8_t const* tile_ptr =
                reinterpret_cast<uint8_t const*>(img.pixels)
                + (corner[1] * img.stride + corner[0]) * n_channels;

            if (target_ == GL_TEXTURE_2D && size != tile_size_)
            {
                alia__shared_ptr<uint8_t> tmp(
                    new uint8_t[tile_size_[0] * tile_size_[1] * n_channels]);
                copy_subimage(tmp.get(), tile_ptr,
                    size[0] * n_channels, size[1],
                    tile_size_[0] * n_channels,
                    img.stride * n_channels, n_channels,
                    n_channels);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, tile_size_[0]);
                glTexImage2D(GL_TEXTURE_2D, 0, internal_format,
                    tile_size_[0], tile_size_[1], 0, image_format,
                    GL_UNSIGNED_BYTE, tmp.get());
            }
            else
            {
                glPixelStorei(GL_UNPACK_ROW_LENGTH, img.stride);
                glTexImage2D(target_, 0, internal_format,
                    size[0], size[1], 0, image_format,
                    GL_UNSIGNED_BYTE, tile_ptr);
            }

            glTexParameteri(target_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(target_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            check_errors();

            glTexParameteri(target_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(target_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Apparently the above sometimes fails, but we can harmlessly
            // ignore this.
            while (glGetError() != GL_NO_ERROR)
                ;
        }
    }
}

void tiled_texture::replace(image_interface const& img)
{
    assert(img.size == image_size_);

    GLenum image_format = get_gl_format(img.format);
    unsigned const n_channels = get_channel_count(img.format);

    for (unsigned i = 0; i < n_tiles_[1]; i++)
    {
        for (unsigned j = 0; j < n_tiles_[0]; j++)
        {
            glBindTexture(GL_TEXTURE_2D,
                texture_names_[i * n_tiles_[0] + j]);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // the region of the image that this tile covers
            vector<2,unsigned> corner =
                make_vector(tile_size_[0] * j, tile_size_[1] * i);
            vector<2,unsigned> size = make_vector(
                (j == n_tiles_[0] - 1) ? last_tile_size_[0] : tile_size_[0],
                (i == n_tiles_[1] - 1) ? last_tile_size_[1] : tile_size_[1]);

            unsigned n_channels = get_channel_count(img.format);
            uint8_t const* tile_ptr =
                reinterpret_cast<uint8_t const*>(img.pixels)
                + (corner[1] * img.stride + corner[0]) * n_channels;

            if (target_ == GL_TEXTURE_2D && size != tile_size_)
            {
                alia__shared_ptr<uint8_t> tmp(
                    new uint8_t[tile_size_[0] * tile_size_[1] * n_channels]);
                copy_subimage(tmp.get(), tile_ptr,
                    size[0] * n_channels, size[1],
                    tile_size_[0] * n_channels,
                    img.stride * n_channels, n_channels,
                    n_channels);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, tile_size_[0]);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tile_size_[0],
                    tile_size_[1], image_format, GL_UNSIGNED_BYTE,
                    tmp.get());
            }
            else
            {
                glPixelStorei(GL_UNPACK_ROW_LENGTH, img.stride);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size[0],
                    size[1], image_format, GL_UNSIGNED_BYTE,
                    tile_ptr);
            }
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
            ctx_->impl_->pending_texture_deletions.push_back(*i);
        }
    }
}

void tiled_texture::draw(
    surface& surface,
    box<2,double> const& surface_region,
    box<2,double> const& image_region,
    rgba8 const& color)
{
    glEnable(target_);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    rgba8 resolved_color = interpolate(rgba8(0, 0, 0, 0), color,
        static_cast<opengl_surface&>(surface).opacity());
    glColor4ub(resolved_color.r, resolved_color.g, resolved_color.b,
        resolved_color.a);

    double scale_x = surface_region.size[0] / image_region.size[0];
    double scale_y = surface_region.size[1] / image_region.size[1];

    double py0 = 0, py1 = tile_size_[1];
    for (unsigned i = 0; i < n_tiles_[1];
        ++i, py0 = py1, py1 += tile_size_[1])
    {
        double const cpy0 = (std::max)(py0, image_region.corner[1]);
        double const cpy1 = (std::min)(py1, get_high_corner(image_region)[1]);

        if (cpy0 >= cpy1)
            continue;

        double ty0 = cpy0 - py0;
        double ty1 = cpy1 - py0;
        if (target_ == GL_TEXTURE_2D)
        {
            ty0 /= tile_size_[1];
            ty1 /= tile_size_[1];
        }

        double px0 = 0, px1 = tile_size_[0];
        for (unsigned j = 0; j < n_tiles_[0];
            ++j, px0 = px1, px1 += tile_size_[0])
        {
            double const cpx0 = (std::max)(px0, image_region.corner[0]);
            double const cpx1 =
                (std::min)(px1, get_high_corner(image_region)[0]);

            if (cpx0 >= cpx1)
                continue;

            double tx0 = cpx0 - px0;
            double tx1 = cpx1 - px0;
            if (target_ == GL_TEXTURE_2D)
            {
                tx0 /= tile_size_[0];
                tx1 /= tile_size_[0];
            }

            double const x0 = surface_region.corner[0] +
                (cpx0 - image_region.corner[0]) * scale_x;
            double const x1 = surface_region.corner[0] +
                (cpx1 - image_region.corner[0]) * scale_x;
            double const y0 = surface_region.corner[1] +
                (cpy0 - image_region.corner[1]) * scale_y;
            double const y1 = surface_region.corner[1] +
                (cpy1 - image_region.corner[1]) * scale_y;

            glBindTexture(target_, texture_names_[i * n_tiles_[0] + j]);

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

    glDisable(target_);
}

}

static opengl_texture* create_texture(
    opengl_context* ctx, image_interface const& img,
    opengl_texture_flag_set flags)
{
    if (img.size[0] > ctx->impl_->max_texture_size ||
        img.size[1] > ctx->impl_->max_texture_size)
    {
        return new tiled_texture(
            ctx, img,
            make_vector<unsigned>(
                ctx->impl_->max_texture_size,
                ctx->impl_->max_texture_size),
            flags);
    }
    else
        return new simple_texture(ctx, img, flags);
}

// OFFSCREEN BUFFERS

struct offscreen_buffer : offscreen_subsurface
{
    offscreen_buffer(opengl_context& ctx, box<2,unsigned> const& region);

    ~offscreen_buffer();

    bool is_valid() const { return context_version_ == ctx_->impl_->version; }

    box<2,unsigned> region() const { return region_; }

    void blit(
        surface& surface,
        rgba8 const& color);

    // In cases where the OpenGL context is reset without our knowledge, we
    // may actually want to leak the handles we have.
    void leak() { ctx_ = 0; }

    opengl_context* ctx_;
    unsigned context_version_;
    box<2,unsigned> region_;
    GLuint framebuffer_name_, color_texture_name_, renderbuffer_name_;
};

offscreen_buffer::offscreen_buffer(
    opengl_context& ctx, box<2,unsigned> const& region)
{
    ctx_ = &ctx;
    context_version_ = ctx.impl_->version;
    region_ = region;

    // Generate the framebuffer.
    glGenFramebuffersEXT(1, &framebuffer_name_);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_name_);

    // Generate color texture.
    glGenTextures(1, &color_texture_name_);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, color_texture_name_);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, region.size[0],
        region.size[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER,
        GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER,
        GL_NEAREST);

    // Generate a render buffer for the depth and stencil components.
    glGenRenderbuffersEXT(1, &renderbuffer_name_);
    glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer_name_);
    glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH24_STENCIL8_EXT,
        region.size[0], region.size[1]);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
        GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderbuffer_name_);
    glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
        GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderbuffer_name_);

    // Associate the texture as the color component of the framebuffer.
    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
        GL_TEXTURE_RECTANGLE_ARB, color_texture_name_, 0);
    GLenum draw_buffers[1] = { GL_COLOR_ATTACHMENT0_EXT };
    glDrawBuffers(1, draw_buffers);

    // Check if all that succeeded.
    if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) !=
        GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        throw exception("framebuffer creation failed");
    }
    check_errors();
}

offscreen_buffer::~offscreen_buffer()
{
    if (ctx_)
    {
        auto& impl = *ctx_->impl_;
        impl.associated_offscreen_buffers.remove(this);
        impl.pending_framebuffer_deletions.push_back(framebuffer_name_);
        impl.pending_texture_deletions.push_back(color_texture_name_);
        impl.pending_renderbuffer_deletions.push_back(renderbuffer_name_);
    }
}

void offscreen_buffer::blit(
    surface& surface,
    rgba8 const& color)
{
    glEnable(GL_TEXTURE_RECTANGLE_ARB);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, color_texture_name_);

    rgba8 resolved_color = interpolate(rgba8(0, 0, 0, 0), color,
        static_cast<opengl_surface&>(surface).opacity());
    glColor4ub(resolved_color.r, resolved_color.g, resolved_color.b,
        resolved_color.a);

    double const
        tx0 = 0,
        tx1 = region_.size[0],
        ty0 = 0,
        ty1 = region_.size[1];

    double const
        x0 = region_.corner[0],
        x1 = x0 + region_.size[0],
        y0 = region_.corner[1],
        y1 = y0 + region_.size[1];

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

    glDisable(GL_TEXTURE_RECTANGLE_ARB);

    check_errors();
}

static offscreen_buffer*
create_offscreen_buffer(opengl_context* ctx, box<2,unsigned> const& region)
{
    if (!GLEW_ARB_framebuffer_object || !GLEW_ARB_draw_buffers)
        return 0;

    if (region.size[0] > ctx->impl_->max_texture_size ||
        region.size[1] > ctx->impl_->max_texture_size)
    {
        return 0;
    }

    if (!ctx->impl_->texture_rectangle_supported)
        return 0;

    offscreen_buffer* buffer = 0;
    try
    {
        buffer = new offscreen_buffer(*ctx, region);
    }
    catch (...)
    {
    }

    return buffer;
}

// CONTEXT

opengl_context::opengl_context()
{
    impl_ = new impl_data;
    impl_->is_initialized = false;
    impl_->version = 0;
}
opengl_context::~opengl_context()
{
    // Orphan any associated UI objects so they won't try to refer back to this
    // context when they're destroyed.
    for (std::list<opengl_texture*>::iterator
        i = impl_->associated_textures.begin();
        i != impl_->associated_textures.end(); ++i)
    {
        (*i)->leak();
    }
    for (std::list<offscreen_buffer*>::iterator
        i = impl_->associated_offscreen_buffers.begin();
        i != impl_->associated_offscreen_buffers.end(); ++i)
    {
        (*i)->leak();
    }

    delete impl_;
}

void opengl_context::reset()
{
    impl_->is_initialized = false;
    impl_->pending_texture_deletions.clear();
    ++impl_->version;
}

void opengl_context::do_pending_deletions()
{
    if (!impl_->pending_texture_deletions.empty())
    {
        for (std::list<GLuint>::iterator
            i = impl_->pending_texture_deletions.begin();
            i != impl_->pending_texture_deletions.end(); ++i)
        {
            glDeleteTextures(1, &*i);
        }
        impl_->pending_texture_deletions.clear();
    }
    if (!impl_->pending_framebuffer_deletions.empty())
    {
        for (std::list<GLuint>::iterator
            i = impl_->pending_framebuffer_deletions.begin();
            i != impl_->pending_framebuffer_deletions.end(); ++i)
        {
            glDeleteFramebuffersEXT(1, &*i);
        }
        impl_->pending_framebuffer_deletions.clear();
    }
    if (!impl_->pending_renderbuffer_deletions.empty())
    {
        for (std::list<GLuint>::iterator
            i = impl_->pending_renderbuffer_deletions.begin();
            i != impl_->pending_renderbuffer_deletions.end(); ++i)
        {
            glDeleteRenderbuffersEXT(1, &*i);
        }
        impl_->pending_renderbuffer_deletions.clear();
    }
}

// SURFACE

void opengl_surface::initialize_render_state(vector<2,unsigned> const& size)
{
    ctx_->do_pending_deletions();

    if (!ctx_->impl_->is_initialized)
    {
        GLenum err = glewInit();
        if (GLEW_OK != err)
            throw exception("GLEW initialization failed");

        int max_texture_size;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
        ctx_->impl_->max_texture_size = max_texture_size;

        ctx_->impl_->texture_rectangle_supported =
            is_opengl_extension_in_list(
                (char const*) glGetString(GL_EXTENSIONS),
                "GL_ARB_texture_rectangle");

        ctx_->impl_->is_initialized = true;
    }

    glViewport(0, 0, size[0], size[1]);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, size[0], size[1], 0, -10000, 10000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, size[0], size[1]);
    clip_region_ = box<2,double>(make_vector(0., 0.), vector<2,double>(size));

    glDisable(GL_LIGHTING);

    size_ = size;
    opacity_ = 1;
}

void opengl_surface::cache_image(
    cached_image_ptr& data,
    image_interface const& img)
{
    this->cache_image(data, img, NO_FLAGS);
}

void opengl_surface::cache_image(
    cached_image_ptr& data,
    image_interface const& img,
    opengl_texture_flag_set flags)
{
    assert(!data || dynamic_cast<opengl_texture*>(data.get()));
    opengl_texture* t = static_cast<opengl_texture*>(data.get());

    // If the texture is for a previous instance of the context, abandon it.
    if (t && t->context_version_ != ctx_->impl_->version)
    {
        t->leak();
        t = 0;
    }

    // If the existing texture is compatible, reuse it.
    if (t && t->size() == img.size && t->format_ == img.format)
    {
        t->replace(img);
        return;
    }

    // Otherwise, create a fresh texture.
    t = create_texture(ctx_, img, flags);
    data.reset(t);
    ctx_->impl_->associated_textures.push_back(t);
}

void static
apply_clip_region(
    box<2,unsigned> const& viewport, box<2,double> const& clip_region)
{
    glScissor(
        int(clip_region.corner[0] - viewport.corner[0] + 0.5),
        int((viewport.corner[1] + viewport.size[1]) -
            (clip_region.corner[1] + clip_region.size[1]) + 0.5),
        int(clip_region.size[0] + 0.5), int(clip_region.size[1] + 0.5));
}

void static
set_active_viewport(
    box<2,unsigned> const& viewport, box<2,double> const& clip_region)
{
    glViewport(0, 0, viewport.size[0], viewport.size[1]);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewport.corner[0], viewport.corner[0] + viewport.size[0],
        viewport.corner[1] + viewport.size[1], viewport.corner[1],
        -10000, 10000);

    apply_clip_region(viewport, clip_region);
}

void opengl_surface::generate_offscreen_subsurface(
    offscreen_subsurface_ptr& subsurface,
    box<2,unsigned> const& region)
{
    assert(!subsurface || dynamic_cast<offscreen_buffer*>(subsurface.get()));
    offscreen_buffer* buffer =
        static_cast<offscreen_buffer*>(subsurface.get());

    // If the buffer is for a previous instance of the context, abandon it.
    if (buffer && buffer->context_version_ != ctx_->impl_->version)
    {
        buffer->leak();
        buffer = 0;
    }

    // If the existing buffer is compatible, reuse it.
    if (buffer && buffer->region().size == region.size)
    {
        buffer->region_ = region;
    }
    // Otherwise, create a new one.
    else
    {
        buffer = create_offscreen_buffer(ctx_, region);
        subsurface.reset(buffer);
        if (buffer)
            ctx_->impl_->associated_offscreen_buffers.push_back(buffer);
    }

    if (buffer)
    {
        // Activate the buffer.
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, buffer->framebuffer_name_);
        set_active_viewport(buffer->region_, clip_region_);

        // And clear it.
        glClearColor(0, 0, 0, 0);
        glClearDepth(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

        check_errors();

        // We should also restore the proper active buffer, but since buffers
        // are always used immediately after being created, that doesn't
        // actually matter. (Probably these steps should be merged in the API.)
    }
}

void opengl_surface::set_active_subsurface(offscreen_subsurface* subsurface)
{
    assert(!subsurface || dynamic_cast<offscreen_buffer*>(subsurface));
    if (subsurface)
    {
        offscreen_buffer* buffer = static_cast<offscreen_buffer*>(subsurface);
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, buffer->framebuffer_name_);
        set_active_viewport(buffer->region_, clip_region_);
    }
    else
    {
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        set_active_viewport(make_box(make_vector(0u, 0u), size_),
            clip_region_);
    }
    check_errors();
    active_subsurface_ = subsurface;
}

void opengl_surface::set_transformation_matrix(matrix<3,3,double> const& m)
{
    double gl_matrix[16] = {
        m(0,0), m(1,0), 0, m(2,0),
        m(0,1), m(1,1), 0, m(2,1),
             0,      0, 1,      0,
        m(0,2), m(1,2), 0, m(2,2) };
    glLoadMatrixd(gl_matrix);
}
void opengl_surface::set_clip_region(box<2,double> const& region)
{
    assert(region.size[0] >= 0 && region.size[1] >= 0);
    box<2,unsigned> active_viewport =
        active_subsurface_ ? active_subsurface_->region() :
            make_box(make_vector(0u, 0u), size_);
    apply_clip_region(active_viewport, region);
    clip_region_ = region;
}

void opengl_surface::draw_filled_box(
    rgba8 const& color, box<2,double> const& box)
{
    // Draw the box using a uniform texture.
    // There are obviously simpler ways to draw filled boxes in OpenGL, but
    // testing shows that this is actually faster. Perhaps switching between
    // textured and non-textured quads slows down the rendering.
    if (!is_valid(ctx_->impl_->uniform_image))
    {
        skia_renderer renderer(
            *this, ctx_->impl_->uniform_image, make_vector<int>(3, 3));
        SkPaint paint;
        paint.setFlags(SkPaint::kAntiAlias_Flag);
        set_color(paint, rgba8(0xff, 0xff, 0xff, 0xff));
        SkRect sr;
        sr.fLeft = 0;
        sr.fRight = SkIntToScalar(3);
        sr.fTop = 0;
        sr.fBottom = SkIntToScalar(3);
        renderer.canvas().drawRect(sr, paint);
        renderer.cache();
    }
    ctx_->impl_->uniform_image->draw(
        *this, box,
        alia::box<2,double>(make_vector(1., 1.), make_vector(1., 1.)),
        color);
}

#ifdef WIN32

void disable_vsync()
{
    if (WGLEW_EXT_swap_control)
        wglSwapIntervalEXT(0);
}

#endif

}
