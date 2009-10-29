#include <alia/scrollable_region.hpp>
#include <alia/scrollbar.hpp>
#include <alia/layout.hpp>
#include <alia/context.hpp>
#include <alia/artist.hpp>
#include <alia/input_events.hpp>
#include <alia/transformations.hpp>

namespace alia {

struct scrolled_region_data
{
    scrollbar_data sb_data[2];
    artist_data_ptr junction_data;
};

static void do_scrollbar_pair(context& ctx, scrolled_region_data& data,
    box2i& window, point2i& position, box2i const& region,
    vector2i const& content_size, vector2i const& line_increment,
    vector2i const& page_increment, unsigned axes, vector2i const& unavailable)
{
    artist& artist = *ctx.artist;

    window = region;
    switch (ctx.event->type)
    {
     case REFRESH_EVENT:
        refresh_scrollbar_data(ctx, data.sb_data[0]);
        refresh_scrollbar_data(ctx, data.sb_data[1]);
        return;
     case LAYOUT_PASS_0:
        return;
     case LAYOUT_PASS_1:
        if ((axes & 2) != 0)
        {
            window.size[0] -= artist.get_scrollbar_width();
            return;
        }
    }

    int sb_width = artist.get_scrollbar_width();
    bool sb_on[2] = { false, false };

    if ((axes & 1) != 0 && window.size[0] < content_size[0])
    {
        sb_on[0] = true;
        window.size[1] -= sb_width;
    }
    if ((axes & 2) != 0 && window.size[1] < content_size[1])
    {
        sb_on[1] = true;
        window.size[0] -= sb_width;
    }
    if ((axes & 1) != 0 && !sb_on[0] && window.size[0] < content_size[0])
    {
        sb_on[0] = true;
        window.size[1] -= sb_width;
    }

    for (int i = 0; i < 2; ++i)
    {
        if (sb_on[i])
        {
            int const other_i = 1 - i;

            point2i corner;
            corner[other_i] = region.corner[other_i] + window.size[other_i];
            corner[i] = region.corner[i];

            vector2i size;
            size[other_i] = artist.get_scrollbar_width();
            size[i] = window.size[i];

            position[i] = do_scrollbar(ctx, data.sb_data[i],
                box2i(corner, size), i, content_size[i],
                window.size[i] - unavailable[i], position[i],
                line_increment[i], page_increment[i]);
        }
        else
            position[i] = 0;
    }

    if (sb_on[0] && sb_on[1])
    {
        artist.draw_scrollbar_junction(data.junction_data,
            get_high_corner(window));
    }
}

static unsigned const line_size = 50;

struct scrollable_region::data
{
    data() : scroll_position(0, 0), minimum_size(0, 0) {}

    point2i scroll_position;

    alia::scrolled_region_data scrolled_region_data;
    vector2i content_size, minimum_size;

    alia::layout_data layout_data;

    unsigned axes;
};

bool scrollable_region::is_relevant() const
{
    return layout_.is_relevant();
}

vector2i scrollable_region::get_content_size() const
{
    return data_->content_size;
}

void scrollable_region::begin(context& ctx, layout const& layout_spec,
    flag_set flags, region_id id)
{
    ctx_ = &ctx;
    data_ = get_data<data>(ctx);
    id_ = id ? id : get_region_id(ctx);
    axes_ = 0;
    layout_spec_ = layout_spec;
    flags_ = flags;
    active_ = false;

    if (flags & HORIZONTAL)
        axes_ |= 1;
    if (flags & VERTICAL)
        axes_ |= 2;
    if (axes_ == 0)
        axes_ = 3;

    if (ctx_->event->category == LAYOUT_CATEGORY &&
        get_event<layout_event>(*ctx_).active_logic)
    {
        resolved_layout_spec resolved;
        resolve_layout_spec(*ctx_, &resolved, layout_spec_,
            resolve_size(*ctx_, layout_spec_.size),
            widget_layout_info(data_->minimum_size, 0, 0, data_->minimum_size,
                FILL, false));
        switch (ctx_->event->type)
        {
         case REFRESH_EVENT:
            diff_widget_location(*ctx_, data_->layout_data);
            if (axes_ != data_->axes)
            {
                record_layout_change(*ctx_, data_->layout_data);
                data_->axes = axes_;
            }
            break;
         case LAYOUT_PASS_1:
            data_->layout_data.assigned_region.size[0] =
                get_assigned_width(*ctx_, resolved);
            break;
         case LAYOUT_PASS_2:
            get_assigned_region(*ctx_, &data_->layout_data.assigned_region,
                resolved);
            record_layout(*ctx_, data_->layout_data, resolved);
            break;
        }
    }

    do_scrollbar_pair(*ctx_, data_->scrolled_region_data, window_region_,
        data_->scroll_position, data_->layout_data.assigned_region,
        data_->content_size, vector2i(line_size, line_size),
        vector2i(-1, -1), axes_, vector2i(0, 0));

    if (ctx_->event->category == REGION_CATEGORY)
        do_region(*ctx_, id_, window_region_);

    scr_.begin(ctx);
    scr_.set(window_region_);
    st_.begin(ctx);
    st_.set(translation(vector2d(window_region_.corner -
        data_->scroll_position)));

    vector2i content_size(
        (std::max)(window_region_.size[0], data_->content_size[0]),
        (std::max)(window_region_.size[1], data_->content_size[1]));
    overlay_.begin(ctx, box2i(point2i(0, 0), content_size));

    layout_.begin(ctx, default_layout, VERTICAL);

    active_ = true;
}

void scrollable_region::end()
{
    if (!active_)
        return;
    active_ = false;

    layout_.end();
    overlay_.end();
    scr_.end();
    st_.end();

    switch (ctx_->event->category)
    {
     case LAYOUT_CATEGORY:
        if (get_event<layout_event>(*ctx_).active_logic)
        {
            int which_pass = -1;
            switch (ctx_->event->type)
            {
             case LAYOUT_PASS_0:
                which_pass = 0;
                break;
             case LAYOUT_PASS_1:
                which_pass = 1;
                break;
            }
            if (which_pass != -1)
            {
                artist& artist = *ctx_->artist;

                int content_size = overlay_.get_minimum_size()[which_pass];
                data_->content_size[which_pass] = content_size;

                // TODO: Fix the GREEDY hack.
                int minimum_size;
                if ((flags_ & GREEDY) != 0)
                {
                    minimum_size = which_pass == 0 ?
                        content_size + artist.get_scrollbar_width() :
                        content_size;
                }
                else
                {
                    minimum_size =
                        (axes_ & (1 << which_pass)) != 0
                      ? (std::min)(content_size,
                            artist.get_scrollbar_button_length() * 2 +
                            artist.get_minimum_scrollbar_thumb_length() * 3 +
                            artist.get_scrollbar_width())
                      : content_size + artist.get_scrollbar_width();
                }

                data_->minimum_size[which_pass] = minimum_size;
            }
            resolved_layout_spec resolved;
            resolve_layout_spec(*ctx_, &resolved, layout_spec_,
                resolve_size(*ctx_, layout_spec_.size),
                widget_layout_info(data_->minimum_size, 0, 0,
                    data_->minimum_size, FILL, false));
            switch (ctx_->event->type)
            {
             case REFRESH_EVENT:
                diff_layout_spec(*ctx_, data_->layout_data, resolved);
                break;
             case LAYOUT_PASS_0:
                request_horizontal_space(*ctx_, resolved);
                break;
             case LAYOUT_PASS_1:
                request_vertical_space(*ctx_, resolved);
                break;
            }
        }
        break;

     case REGION_CATEGORY:
        if (ctx_->event->type == MAKE_REGION_VISIBLE &&
            layout_.is_on_target_path())
        {
            make_region_visible_event& e =
                get_event<make_region_visible_event>(*ctx_);
            point2i region_ul = e.region.corner,
                region_lr = get_high_corner(e.region);
            point2i window_ul = data_->scroll_position,//window_region_.corner,
                window_lr = window_ul + window_region_.size;//get_high_corner(window_region_);
            for (int i = 0; i < 2; ++i)
            {
                if (e.region.size[i] <= window_region_.size[i])
                {
                    if (region_ul[i] < window_ul[i] &&
                        region_lr[i] < window_lr[i])
                    {
                        data_->scroll_position[i] -=
                            window_ul[i] - region_ul[i];
                    }
                    else if (region_ul[i] > window_ul[i] &&
                        region_lr[i] > window_lr[i])
                    {
                        data_->scroll_position[i] +=
                            (std::min)(region_ul[i] - window_ul[i],
                                region_lr[i] - window_lr[i]);
                    }
                }
                else
                {
                    if (region_lr[i] < window_ul[i] ||
                        region_ul[i] >= window_lr[i])
                    {
                        data_->scroll_position[i] -=
                            window_ul[i] - region_ul[i];
                    }
                }
            }
            e.region.corner += vector2i(window_region_.corner);
        }
        break;

     case INPUT_CATEGORY:
        if (ctx_->event->type == BUTTON_DOWN_EVENT &&
            is_region_hot(*ctx_, id_))
        {
            set_focus(*ctx_, id_);
        }
        if (layout_.is_on_target_path() || id_has_focus(*ctx_, id_))
        {
            key_event_info info;
            if (detect_key_press(*ctx_, &info) && info.mods == 0)
            {
                switch (info.code)
                {
                 case KEY_UP:
                    data_->scroll_position[1] -= line_size;
                    break;
                 case KEY_DOWN:
                    data_->scroll_position[1] += line_size;
                    break;
                 case KEY_PAGEUP:
                    data_->scroll_position[1] -= (std::max)(
                        window_region_.size[1] - line_size, line_size);
                    break;
                 case KEY_PAGEDOWN:
                    data_->scroll_position[1] += (std::max)
                        (window_region_.size[1] - line_size, line_size);
                    break;
                 case KEY_LEFT:
                    data_->scroll_position[0] -= line_size;
                    break;
                 case KEY_RIGHT:
                    data_->scroll_position[0] += line_size;
                    break;
                 case KEY_HOME:
                    data_->scroll_position[1] = 0;
                    break;
                 case KEY_END:
                    data_->scroll_position[1] = data_->content_size[1] -
                        window_region_.size[1];
                    break;
                }
            }
        }
        if (mouse_is_inside_region(*ctx_, window_region_))
        {
            int wheel_movement = detect_wheel_movement(*ctx_);
            if (wheel_movement > 0 && data_->scroll_position[1] > 0 ||
                wheel_movement < 0 && data_->scroll_position[1] <
                data_->content_size[1] - window_region_.size[1])
            {
                data_->scroll_position[1] -= wheel_movement * line_size;
                acknowledge_input_event(*ctx_);
            }
        }
        break;
    }

    // TODO: merge with rest
    //if (ctx.event->type == JUMP_TO_LOCATION)
    //{
    //    jump_event& e = get_event<jump_event>(ctx);
    //    if (e.state == jump_event::JUMPING)
    //        
    //}
}

}
