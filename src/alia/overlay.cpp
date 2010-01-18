#include <alia/overlay.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>

namespace alia {

struct overlay::data
{
    data() : minimum_size(0, 0), region(point2i(0, 0), vector2i(0, 0)),
        refresh_region(point2i(0, 0), vector2i(0, 0)), children(0) {}
    alia::layout_data layout_data;
    vector2i minimum_size;
    int minimum_ascent, minimum_descent;
    box2i region, refresh_region;
    alia::layout_data* children;
};

// overlay::logic

void overlay::logic::begin(data& data, box2i const& region)
{
    data_ = &data;
    region_ = region;
}
void overlay::logic::end()
{
}
void overlay::logic::request_horizontal_space_for_node(int minimum_width,
    float proportion)
{
    data_->minimum_size[0] = minimum_width;
}
int overlay::logic::get_width_for_node(int minimum_width,
    float proportion)
{
    return region_.size[0];
}
void overlay::logic::request_vertical_space_for_node(
    int minimum_height, int minimum_ascent, int minimum_descent,
    float proportion)
{
    data_->minimum_size[1] = minimum_height;
    data_->minimum_ascent = minimum_ascent;
    data_->minimum_descent = minimum_descent;
}
void overlay::logic::get_region_for_node(box2i* region, int* baseline_y,
    vector2i const& minimum_size, float proportion)
{
    *region = region_;
    *baseline_y = 0;
}

// overlay

void overlay::begin(context& ctx, point2i const& position)
{
    data& d = *get_data<data>(ctx);
    begin(ctx, d, box2i(position, d.minimum_size));
}
void overlay::begin(context& ctx, box2i const& region)
{
    begin(ctx, *get_data<data>(ctx), region);
}
void overlay::begin(context& ctx, data& data, box2i const& region)
{
    active_ = false;

    if (ctx.event->category == LAYOUT_CATEGORY)
    {
        ctx_ = &ctx;
        layout_event& e = get_event<layout_event>(ctx);
        switch (e.type)
        {
         case REFRESH_EVENT:
          {
            if (&data.layout_data == e.active_data)
                assert(&data.layout_data != e.active_data);
            data.layout_data.parent = e.active_data;
            if (region != data.refresh_region)
            {
                record_layout_change(ctx, data.layout_data);
                data.refresh_region = region;
            }
            break;
          }
         case LAYOUT_PASS_1:
            data.region.size[0] = region.size[0];
            break;
         case LAYOUT_PASS_2:
            data.region = region;
            data.layout_data.dirty = false;
            break;
        }
        old_logic_ = e.active_logic;
        old_data_ = e.active_data;
        old_next_ptr_ = e.next_ptr;
        e.active_logic = &logic_;
        e.active_data = &data.layout_data;
        e.next_ptr = &data.children;
        logic_.begin(data, region);
        active_ = true;
    }
}
void overlay::end()
{
    if (active_)
    {
        layout_event& e = get_event<layout_event>(*ctx_);
        e.active_logic = old_logic_;
        e.active_data = old_data_;
        e.next_ptr = old_next_ptr_;
        active_ = false;
    }
}
int overlay::get_minimum_ascent() const
{
    return logic_.data_->minimum_ascent;
}
int overlay::get_minimum_descent() const
{
    return logic_.data_->minimum_descent;
}
vector2i const& overlay::get_minimum_size() const
{
    return logic_.data_->minimum_size;
}

}
