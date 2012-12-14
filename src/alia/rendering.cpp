#include <alia/rendering.hpp>

namespace alia {

line_stipple no_line(1, 0), solid_line(1, 0xffff), dashed_line(10, 0x5555),
    dotted_line(3, 0x5555);

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

void caching_renderer::begin(caching_renderer_data& data, surface& surface,
    id_interface const& content_id, box<2,int> const& region)
{
    data_ = &data;
    surface_ = &surface;
    region_ = region;

    refresh_keyed_data(data,
        combine_ids(ref(content_id), make_id(region.size)));

    needs_rendering_ = !data.is_valid || !is_valid(data.value);
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

        surface_ = 0;
    }
}

}
