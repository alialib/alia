#include <alia/opengl/context.hpp>
#include <alia/opengl/simple_texture.hpp>
#include <alia/opengl/tiled_texture.hpp>
#include <map>
#include <list>
#include <memory>
#include <boost/shared_ptr.hpp>

namespace alia { namespace opengl {

struct image_data;

struct context::impl_data
{
    std::list<image_data*> associated_data;
    std::list<GLuint> pending_deletions;
    // This is incremented each time the textures are invalidated.
    // When a texture is created, it is assigned the current version number.
    // Thus, only textures with the current version number are valid.
    unsigned version;
};

struct image_data : cached_image
{
    context* ctx;

    boost::shared_ptr<opengl::texture> texture;
    unsigned context_version;

    pixel_format format;
    unsigned flags;

    ~image_data();

    vector2i size() const { return texture->size(); }

    bool valid() const { return context_version == ctx->impl_->version; }

    void draw(point2d const& p, rgba8 const& color)
    {
        glColor4ub(color.r, color.g, color.b, color.a);
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        texture->draw(p);
        glDisable(GL_TEXTURE_2D);
    }
    void draw_region(point2d const& p, box2d const& region,
        rgba8 const& color)
    {
        glColor4ub(color.r, color.g, color.b, color.a);
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        texture->draw_region(p, region);
        glDisable(GL_TEXTURE_2D);
    }
};

image_data::~image_data()
{
    if (ctx)
        ctx->impl_->associated_data.remove(this);
}

context::context()
{
    impl_ = new impl_data;
    impl_->version = 0;
}
context::~context()
{
    // Orphan the textures so they won't try to refer back to this context when
    // they're destroyed.
    for (std::list<image_data*>::iterator i = impl_->associated_data.begin();
        i != impl_->associated_data.end(); ++i)
    {
        if ((*i)->texture)
            (*i)->texture->leak();
        (*i)->ctx = 0;
    }
    delete impl_;
}

void context::invalidate_textures()
{ ++impl_->version; }

void context::add_pending_texture_deletion(GLuint name)
{
    impl_->pending_deletions.push_back(name);
}

void context::do_pending_texture_deletions()
{
    for (std::list<GLuint>::iterator i = impl_->pending_deletions.begin();
        i != impl_->pending_deletions.end(); ++i)
    {
        glDeleteTextures(1, &*i);
    }
    impl_->pending_deletions.clear();
}

static image_data* create_image_data(context* ctx, cached_image_ptr& data)
{
    image_data* d = new image_data;
    d->ctx = 0;
    data.reset(d);
    // Now the data is owned and in a state where it can safely be destroyed,
    // in case push_back throws.
    ctx->impl_->associated_data.push_back(d);
    d->ctx = ctx;
    return d;
}

static texture* create_texture(context* ctx, image_interface const& img,
    unsigned flags)
{
    int max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    if (img.size[0] > max_texture_size || img.size[1] > max_texture_size)
    {
        return new tiled_texture(ctx, img, vector2i(max_texture_size,
            max_texture_size), flags);
    }
    else
        return new simple_texture(ctx, img, flags);
}

void context::cache_image(
    cached_image_ptr& data,
    image_interface const& img,
    unsigned flags)
{
    image_data* d = dynamic_cast<image_data*>(data.get());

    // If the texture is outdated, abandon it.
    if (d && d->context_version != impl_->version)
    {
        d->texture->leak();
        d = 0;
    }

    // If the existing data is compatible, reuse it.
    if (d && d->size() == img.size && d->format == img.format &&
        d->flags == flags)
    {
        d->texture->replace(img);
        return;
    }

    // Otherwise, create a fresh texture.
    boost::shared_ptr<opengl::texture> texture(
        create_texture(this, img, flags));
    d = create_image_data(this, data);
    d->flags = flags;
    d->format = img.format;
    d->context_version = impl_->version;
    swap(d->texture, texture);
}

}}
