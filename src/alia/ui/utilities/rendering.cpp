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

void caching_renderer::begin(caching_renderer_data& data, surface& surface,
    geometry_context& geometry,
    id_interface const& content_id, box<2,int> const& region)
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

style_search_path const*
get_control_style_path(ui_context& ctx,
    stateless_control_style_path_storage* storage,
    char const* control_type)
{
    return
        add_substyle_to_path(
            &storage->nodes[1],
            ctx.style.path, 
            add_substyle_to_path(&storage->nodes[0], ctx.style.path, 0,
                "control"),
            control_type);
}

style_search_path const*
get_control_style_path(ui_context& ctx, control_style_path_storage* storage,
    char const* control_type, widget_state state)
{
    return
        add_substyle_to_path(
            &storage->storage[1],
            ctx.style.path,
            add_substyle_to_path(&storage->storage[0], ctx.style.path, 0,
                "control", state),
            control_type, state);
}

layout_vector
get_box_control_size(ui_context& ctx, char const* control_type)
{
    ALIA_GET_CACHED_DATA(keyed_data<layout_vector>)
    refresh_keyed_data(data, *ctx.style.id);
    if (!is_valid(data))
    {
        stateless_control_style_path_storage storage;
        style_search_path const* path =
            get_control_style_path(ctx, &storage, "check-box");
        set(data,
            as_layout_size(eval_absolute_size_spec(ctx,
                get_property(path, "size",
                    absolute_size_spec_2d(
                        absolute_size_spec(1.2f, EM),
                        absolute_size_spec(1.2f, EM))))) +
            2 * as_layout_size(eval_absolute_size_spec(ctx,
                get_property(path, "border-width",
                    absolute_size_spec_2d(
                        absolute_size_spec(0, PIXELS),
                        absolute_size_spec(0, PIXELS))))));
    }
    return get(data);
}

layout_vector
get_box_control_size(ui_context& ctx, keyed_data<layout_vector>& data,
    char const* control_type)
{
    refresh_keyed_data(data, *ctx.style.id);
    if (!is_valid(data))
    {
        stateless_control_style_path_storage storage;
        style_search_path const* path =
            get_control_style_path(ctx, &storage, "check-box");
        set(data,
            as_layout_size(eval_absolute_size_spec(ctx,
                get_property(path, "size",
                    absolute_size_spec_2d(
                        absolute_size_spec(1.2f, EM),
                        absolute_size_spec(1.2f, EM))))) +
            2 * as_layout_size(eval_absolute_size_spec(ctx,
                get_property(path, "border-width",
                    absolute_size_spec_2d(
                        absolute_size_spec(0, PIXELS),
                        absolute_size_spec(0, PIXELS))))));
    }
    return get(data);
}

}
