#include <alia/collapsible_content.hpp>
#include <alia/layout.hpp>
#include <alia/context.hpp>
#include <alia/transformations.hpp>
#include <alia/input_utils.hpp>
#include <alia/timer.hpp>
#include <alia/input_events.hpp>

namespace alia {

enum state_t { UNINITIALIZED, COLLAPSED, COLLAPSING, EXPANDED, EXPANDING };

struct collapsible_content::data
{
    data() : content_size(0, 0), state(UNINITIALIZED), size(0) {}
    vector2i content_size;
    state_t state;
    int size;
    alia::layout_data layout_data;
};

void collapsible_content::begin(context& ctx, bool expanded,
    layout const& layout_spec)
{
    ctx_ = &ctx;
    data_ = get_data<data>(ctx);
    layout_spec_ = layout_spec;
    // TODO: check that layout spec doesn't GROW vertically

    active_ = false;

    region_id id = get_region_id(ctx);

    if (ctx_->event->type == REFRESH_EVENT)
    {
        switch (data_->state)
        {
         case UNINITIALIZED:
            data_->state = expanded ? EXPANDED : COLLAPSED;
            break;
         case COLLAPSED:
            if (expanded)
            {
                data_->size = 0;
                data_->state = EXPANDING;
                start_timer(ctx, id, 0);
            }
            break;
         case COLLAPSING:
            if (expanded)
            {
                data_->state = EXPANDING;
                start_timer(ctx, id, 0);
            }
            else if (data_->size <= 0)
                data_->state = COLLAPSED;
            break;
         case EXPANDED:
            if (!expanded)
            {
                data_->size = data_->content_size[1];
                data_->state = COLLAPSING;
                start_timer(ctx, id, 0);
            }
            break;
         case EXPANDING:
            if (!expanded)
            {
                data_->state = COLLAPSING;
                start_timer(ctx, id, 0);
            }
            else if (data_->size >= data_->content_size[1])
                data_->state = EXPANDED;
            break;
        }
    }

    do_children_ = data_->state != COLLAPSED;

    if (is_timer_done(ctx, id))
    {
        timer_event& e = get_event<timer_event>(ctx);
        int delta = int((e.now - e.time_requested) *
            std::max<float>(1.5, float(data_->content_size[1]) / 200));
        switch (data_->state)
        {
         case COLLAPSING:
            data_->size -= delta;
            if (data_->size < 0)
                data_->size = 0;
            else if (data_->size > 0)
                start_timer(ctx, id, 1);
            break;
         case EXPANDING:
            data_->size += delta;
            if (data_->size > data_->content_size[1])
                data_->size = data_->content_size[1];
            else if (data_->size < data_->content_size[1])
                start_timer(ctx, id, 1);
            break;
        }
    }

    if (ctx_->event->category == LAYOUT_CATEGORY &&
        get_event<layout_event>(*ctx_).active_logic)
    {
        resolved_layout_spec resolved;
        resolve_layout_spec(*ctx_, &resolved, layout_spec_,
            resolve_size(*ctx_, layout_spec_.size),
            widget_layout_info(
                vector2i(data_->content_size[0], data_->size),
                0, 0,
                vector2i(data_->content_size[0], data_->size),
                FILL, false));
        switch (ctx_->event->type)
        {
         case REFRESH_EVENT:
            diff_widget_location(*ctx_, data_->layout_data);
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

    scr_.begin(ctx);
    scr_.set(data_->layout_data.assigned_region);
    st_.begin(ctx);
    st_.set(translation(vector2d(data_->layout_data.assigned_region.corner -
        point2i(0, data_->content_size[1] - data_->size))));

    overlay_.begin(ctx, box2i(point2i(0, 0),
        vector2i(data_->layout_data.assigned_region.size[0],
            data_->content_size[1])));

    layout_.begin(ctx, default_layout, VERTICAL);

    active_ = true;
}

void collapsible_content::end()
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
            switch (ctx_->event->type)
            {
             case LAYOUT_PASS_0:
                data_->content_size[0] = overlay_.get_minimum_size()[0];
                break;
             case LAYOUT_PASS_1:
                data_->content_size[1] = overlay_.get_minimum_size()[1];
                if (data_->state == EXPANDED)
                    data_->size = data_->content_size[1];
                break;
            }
            resolved_layout_spec resolved;
            resolve_layout_spec(*ctx_, &resolved, layout_spec_,
                resolve_size(*ctx_, layout_spec_.size),
                widget_layout_info(
                    vector2i(data_->content_size[0], data_->size),
                    0, 0,
                    vector2i(data_->content_size[0], data_->size),
                    FILL, false));
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
    }
}

}
