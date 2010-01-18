#include <alia/layout_object.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>

namespace alia {

box2i const& layout_object::get_region() const
{
    return data_->layout_data.assigned_region;
}

bool layout_object::is_relevant() const
{
    switch (ctx_->event->culling_type)
    {
     case SCREEN_CULLING:
        return overlapping(get_region(),
            ctx_->pass_state.integer_untransformed_clip_region);
     case POINT_CULLING:
      {
        box2d intersection;
        return
            // TODO: Use integers instead?
            compute_intersection(&intersection, box2d(get_region()),
                ctx_->pass_state.untransformed_clip_region)
         && is_inside(intersection,
                get_event<point_event>(*ctx_).untransformed_p);
      }
     case LAYOUT_CULLING:
        return logic_needed_;
     case TARGETED_CULLING:
        return true;
     default:
     case NO_CULLING:
        return true;
    }
}

bool layout_object::contains_target() const
{
    return !already_saw_target_ && get_event<targeted_event>(*ctx_).saw_target;
}

bool layout_object::is_dirty() const
{
    return data_->layout_data.dirty;
}

void layout_object::begin(context& ctx, layout_object_data& data,
    layout const& layout_spec, layout_logic* logic)
{
    ctx_ = &ctx;
    data_ = &data;

    logic_needed_ = false;
    active_ = false;

    // already_saw_target_ is only relevant with targeted culling, but setting it
    // to true on other events makes contains_target() safe for those events.
    already_saw_target_ =
        ctx.event->culling_type == TARGETED_CULLING ?
        get_event<targeted_event>(ctx).saw_target : true;

    if (ctx.event->category == LAYOUT_CATEGORY)
    {
        layout_spec_ = layout_spec;
        logic_needed_ = data.layout_data.dirty;
        layout_event& e = get_event<layout_event>(ctx);
        if (e.active_logic)
        {
            if (e.active_grids)
                logic_needed_ = true;
            if (ctx.event->type != REFRESH_EVENT)
            {
                widget_layout_info info(data.minimum_size, data.minimum_ascent,
                    data.minimum_descent, vector2i(0, 0), FILL, false);
                resolved_layout_spec resolved;
                resolve_layout_spec(ctx, &resolved, layout_spec,
                    resolve_size(ctx, layout_spec.size), info);
                switch (ctx.event->type)
                {
                 case LAYOUT_PASS_1:
                   {
                    int width = get_assigned_width(ctx, resolved);
                    if (width != data.layout_data.assigned_region.size[0])
                    {
                        data.layout_data.assigned_region.size[0] = width;
                        data.layout_data.dirty = true;
                        logic_needed_ = true;
                    }
                    break;
                  }
                 case LAYOUT_PASS_2:
                  {
                    box2i new_region;
                    alia::get_assigned_region(ctx, &new_region, resolved);
                    if (data.layout_data.assigned_region != new_region)
                    {
                        data.layout_data.assigned_region = new_region;
                        data.layout_data.dirty = true;
                        logic_needed_ = true;
                    }
                    break;
                  }
                }
            }
            else
            {
                diff_widget_location(ctx, data.layout_data);
                logic_needed_ = true;
            }
        }
        old_logic_ = e.active_logic;
        old_data_ = e.active_data;
        old_next_ptr_ = e.next_ptr;
        if (logic_needed_)
        {
            e.active_logic = logic;
            e.active_data = &data.layout_data;
            e.next_ptr = &data_->children;
        }
        else
        {
            e.active_logic = 0;
            e.active_data = 0;
            e.next_ptr = 0;
        }
        active_ = true;
    }
}

void layout_object::end()
{
    if (active_)
    {
        active_ = false;
        // end the list of children
        if (logic_needed_)
        {
            switch (ctx_->event->type)
            {
             case REFRESH_EVENT:
              {
                refresh_event& e = get_event<refresh_event>(*ctx_);
                assert(e.next_ptr);
                if (*e.next_ptr)
                    record_layout_change(*ctx_, data_->layout_data);
                break;
              }
             case LAYOUT_PASS_2:
              {
                layout_event& e = get_event<layout_event>(*ctx_);
                assert(e.next_ptr);
                *e.next_ptr = 0;
                break;
              }
            }
        }
        // restore old object
        layout_event& e = get_event<layout_event>(*ctx_);
        e.active_logic = old_logic_;
        e.active_data = old_data_;
        e.next_ptr = old_next_ptr_;
        // do layout
        if (e.active_logic)
        {
            widget_layout_info info(data_->minimum_size, data_->minimum_ascent,
                data_->minimum_descent, vector2i(0, 0), FILL, false);
            resolved_layout_spec resolved;
            resolve_layout_spec(*ctx_, &resolved, layout_spec_,
                resolve_size(*ctx_, layout_spec_.size), info);
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
             case LAYOUT_PASS_2:
                record_layout(*ctx_, data_->layout_data, resolved);
                break;
            }
        }
    }
}

}
