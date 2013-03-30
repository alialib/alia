#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

bool operator==(font const& a, font const& b)
{
    return a.name == b.name && a.size == b.size && a.style == b.style;
}
bool operator!=(font const& a, font const& b)
{
    return !(a == b);
}
bool operator<(font const& a, font const& b)
{
    return a.name < b.name || (a.name == b.name &&
        (a.size < b.size || (a.size == b.size &&
        a.style < b.style)));
}

unsigned get_channel_count(pixel_format fmt)
{
    switch (fmt)
    {
     case RGB:
        return 3;
     case RGBA:
        return 4;
     case GRAY:
     default:
        return 1;
    }
}

void draw_full_image(surface& surface, cached_image_ptr const& image,
    vector<2,double> const& position)
{
    assert(is_valid(image));
    vector<2,double> image_size = vector<2,double>(image->size());
    image->draw(surface, box<2,double>(position, image_size),
        box<2,double>(make_vector<double>(0, 0), image_size));
}

void caching_renderer::begin(
    caching_renderer_data& data, surface& surface,
    geometry_context& geometry, id_interface const& content_id,
    layout_box const& region)
{
    if (is_visible(geometry, box<2,double>(region)))
    {
        refresh_keyed_data(data,
            combine_ids(ref(content_id), make_id(region.size)));
        data_ = &data;
        region_ = region;
        surface_ = &surface;
        needs_rendering_ = !data.is_valid || !is_valid(data.value);
    }
    else
    {
        surface_ = 0;
        needs_rendering_ = false;
    }
}

void caching_renderer::draw()
{
    if (surface_)
    {
        if (data_->is_valid)
        {
            data_->value->draw(*surface_,
                box<2,double>(region_),
                box<2,double>(
                    make_vector(0., 0.),
                    vector<2,double>(region_.size)));
        }
    }
}

void clear_rendering_data(themed_rendering_data& data)
{
    data.theme_id.clear();
    clear_data_block(data.refresh_block);
    clear_data_block(data.drawing_block);
    data.theme_renderer.reset();
}

}
