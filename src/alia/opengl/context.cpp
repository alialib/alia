#include <alia/opengl/context.hpp>
#include <alia/opengl/simple_texture.hpp>
#include <alia/opengl/tiled_texture.hpp>
#include <map>
#include <list>
#include <memory>
#include <boost/shared_ptr.hpp>

namespace alia { namespace opengl {

struct image_key
{
    void const* pixels;
    pixel_format format;
    vector2i size, step;
    unsigned flags;
};

static inline bool operator==(image_key const& a, image_key const& b)
{
    return a.pixels == b.pixels && a.format == b.format &&
        a.size == b.size && a.step == b.step && a.flags == b.flags;
}
static inline bool operator!=(image_key const& a, image_key const& b)
{
    return !(a == b);
}
static inline bool operator<(image_key const& a, image_key const& b)
{
    return a.pixels < b.pixels || a.pixels == b.pixels &&
        (a.format < b.format || a.format == b.format &&
        (a.size[0] < b.size[0] || a.size[0] == b.size[0] &&
        (a.size[1] < b.size[1] || a.size[1] == b.size[1] &&
        (a.step[0] < b.step[0] || a.step[0] == b.step[0] &&
        (a.step[1] < b.step[1] || a.step[1] == b.step[1] &&
        a.flags < b.flags)))));
}

struct texture_record
{
    boost::shared_ptr<opengl::texture> texture;
    unsigned version;
    unsigned ref_count;
    std::map<image_key,texture_record>::iterator iter;
};

struct image_data : cached_image
{
    context* ctx;
    std::list<image_data*>::iterator iter;
    texture_record* record;
    ~image_data();
    vector2i get_size() const { return record->texture->get_size(); }
    void draw(point2d const& p, rgba8 const& color)
    {
        glColor4ub(color.r, color.g, color.b, color.a);
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        record->texture->draw(p);
        glDisable(GL_TEXTURE_2D);
    }
    void draw_region(point2d const& p, box2d const& region,
        rgba8 const& color)
    {
        glColor4ub(color.r, color.g, color.b, color.a);
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        record->texture->draw_region(p, region);
        glDisable(GL_TEXTURE_2D);
    }
};

struct context::impl_data
{
    std::map<image_key,texture_record> cached_images;
    std::list<image_data*> associated_data;
    std::list<GLuint> pending_deletions;
    // This is incremented each time the textures are invalidated.
    // When a texture is created, it is assigned the current version number.
    // Thus, only textures with the current version number are valid.
    unsigned texture_version;
};

image_data::~image_data()
{
    if (ctx)
    {
        if (--record->ref_count == 0)
            ctx->impl_->cached_images.erase(record->iter);
        ctx->impl_->associated_data.erase(iter);
    }
}

context::context()
{
    impl_ = new impl_data;
    impl_->texture_version = 0;
}
context::~context()
{
    // Orphan the textures and image data so they won't try to refer back to
    // this context when they're destroyed.
    for (std::map<image_key,texture_record>::iterator
        i = impl_->cached_images.begin(); i != impl_->cached_images.end(); ++i)
    {
        i->second.texture->ctx_ = 0;
    }
    for (std::list<image_data*>::iterator i = impl_->associated_data.begin();
        i != impl_->associated_data.end(); ++i)
    {
        (*i)->ctx = 0;
    }
    delete impl_;
}

void context::invalidate_textures()
{ ++impl_->texture_version; }

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

static void create_image_data_for_record(context* ctx,
    cached_image_ptr& data, texture_record& r)
{
    image_data* d = new image_data;
    d->ctx = 0;
    data.reset(d);
    // Now the data is owned and in a state where it can safely be destroyed,
    // in case push_back throws.
    ctx->impl_->associated_data.push_back(d);
    d->ctx = ctx;
    d->iter = ctx->impl_->associated_data.end();
    --d->iter;
    d->record = &r;
    ++r.ref_count;
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
    image_key k;
    k.pixels = img.pixels;
    k.format = img.format;
    k.size = img.size;
    k.step = img.step;
    k.flags = flags;

    image_data* d = 0;
    if (data)
    {
        assert(dynamic_cast<image_data*>(data.get()));
        d = static_cast<image_data*>(data.get());
        // If this is already the correct image, then we just need to
        // (possibly) update it with the new version.
        if (d->record->iter->first == k)
        {
            if (d->record->version != img.version)
                d->record->texture->replace(img);
            return;
        }
    }

    // See if the image is already cached.
    std::map<image_key,texture_record>::iterator i =
        impl_->cached_images.find(k);
    if (i != impl_->cached_images.end())
    {
        if (i->second.version != img.version)
            i->second.texture->replace(img);
        create_image_data_for_record(this, data, i->second);
        return;
    }

    // If the underlying texture is not shared, and it's compatible, reuse it.
    if (d && d->record->ref_count == 1)
    {
        image_key const& cached_key = d->record->iter->first;
        if (cached_key.size == img.size && cached_key.format == img.format &&
            cached_key.flags == flags)
        {
            i = impl_->cached_images.insert(std::map<image_key,texture_record>
                ::value_type(k, *d->record)).first;
            i->second.iter = i;
            i->second.version = img.version;
            assert(d->record->iter != i);
            impl_->cached_images.erase(d->record->iter);
            d->record = &i->second;
            d->record->texture->replace(img);
            return;
        }
    }

    // Otherwise, create a fresh texture.
    texture_record r;
    r.texture.reset(create_texture(this, img, flags));
    r.ref_count = 0;
    r.version = img.version;
    i = impl_->cached_images.insert(
        std::map<image_key,texture_record>::value_type(k, r)).first;
    i->second.iter = i;
    create_image_data_for_record(this, data, i->second);
}

}}
