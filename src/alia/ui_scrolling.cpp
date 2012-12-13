#include <alia/ui_library.hpp>
#include <alia/ui_utilities.hpp>
#include <alia/skia.hpp>

namespace alia {

struct scrollbar_renderer : dispatch_interface
{
    virtual int width(ui_context& ctx) const = 0;
    virtual int button_length(ui_context& ctx) const = 0;
    virtual int minimum_thumb_length(ui_context& ctx) const = 0;

    // background
    virtual void draw_background(
        ui_context& ctx, renderer_data_ptr& data, box<2,int> const& rect,
        unsigned axis, unsigned which, widget_state state) const = 0;

    // thumb
    virtual void draw_thumb(
        ui_context& ctx, renderer_data_ptr& data, box<2,int> const& rect,
        unsigned axis, widget_state state) const = 0;

    // button
    virtual void draw_button(
        ui_context& ctx, renderer_data_ptr& data,
        vector<2,int> const& position, unsigned axis, unsigned which,
        widget_state state) const = 0;
};

struct scrollbar_junction_renderer : dispatch_interface
{
    // junction - the little square where two scrollbars meet
    virtual void draw(
        ui_context& ctx, renderer_data_ptr& data,
        box<2,int> const& position) const = 0;
};

struct default_scrollbar_renderer : scrollbar_renderer
{
    struct data_type
    {
        caching_renderer_data thumb;
    };

    int width(ui_context& ctx) const
    {
        return resolve_layout_width(get_layout_traversal(ctx), 0.8f, EM);
    }
    int button_length(ui_context& ctx) const
    {
        return 0;
    }
    int minimum_thumb_length(ui_context& ctx) const
    {
        // Must be larger the width.
        return resolve_layout_width(get_layout_traversal(ctx), 1, EM);
    }

    // background
    void draw_background(
        ui_context& ctx, renderer_data_ptr& data_ptr, box<2,int> const& rect,
        unsigned axis, unsigned which, widget_state state) const
    {
    }

    // thumb
    void draw_thumb(
        ui_context& ctx, renderer_data_ptr& data_ptr, box<2,int> const& rect,
        unsigned axis, widget_state state) const
    {
        data_type* data;
        cast_data_ptr(&data, data_ptr);

        caching_renderer cache(ctx, data->thumb,
            combine_ids(ref(*ctx.style.id), make_id(state)),
            rect);
        if (cache.needs_rendering())
        {
            skia_renderer renderer(ctx, cache.image(), rect.size);

            style_tree const* scrollbar_style =
                find_substyle(ctx.style.path, "scrollbar", state);

            SkPaint paint;
            paint.setFlags(SkPaint::kAntiAlias_Flag);

            set_color(paint, get_color_property(scrollbar_style, "color"));

            SkScalar scrollbar_width = SkScalar(this->width(ctx));
            SkScalar const r = scrollbar_width / SkIntToScalar(2);

            paint.setStrokeWidth(scrollbar_width - SkIntToScalar(2));
            paint.setStrokeCap(SkPaint::kRound_Cap);
            renderer.canvas().drawLine(SkScalar(r), SkScalar(r),
                SkScalar(rect.size[0] - r), SkScalar(rect.size[1] - r), paint);

            renderer.cache();
            cache.mark_valid();
        }
        cache.draw();
    }

    // button
    void draw_button(
        ui_context& ctx, renderer_data_ptr& data_ptr,
        vector<2,int> const& position, unsigned axis, unsigned which,
        widget_state state) const
    {
    }
};

struct default_scrollbar_junction_renderer : scrollbar_junction_renderer
{
    // junction - the little square where two scrollbars meet
    void draw(
        ui_context& ctx, renderer_data_ptr& data_ptr,
        box<2,int> const& rect) const
    {
    }
};

struct scrollbar_data
{
    scrollbar_data() : axis(0), physical_position(0) {}
    unsigned axis;
    int physical_position, drag_start_delta;
    themed_rendering_data<scrollbar_renderer> rendering;
    widget_data background_id_data[2], thumb_id_data, button_id_data[2];
};

int get_scrollbar_width(ui_context& ctx, scrollbar_data const& data)
{
    return data.rendering.renderer->width(ctx);
}

int get_minimum_scrollbar_length(ui_context& ctx, scrollbar_data const& data)
{
    return data.rendering.renderer->minimum_thumb_length(ctx) +
        2 * data.rendering.renderer->button_length(ctx);
}

void refresh_scrollbar_data(ui_context& ctx, scrollbar_data& data)
{
    static default_scrollbar_renderer default_renderer;
    refresh_themed_rendering_data(ctx, data.rendering, &default_renderer);
    //refresh_widget_id(ctx, &data.background_id_data[0]);
    //refresh_widget_id(ctx, &data.background_id_data[1]);
    //refresh_widget_id(ctx, &data.thumb_id_data);
    //refresh_widget_id(ctx, &data.button_id_data[0]);
    //refresh_widget_id(ctx, &data.button_id_data[1]);
}

class scrollbar
{
 public:
    scrollbar(ui_context& ctx, scrollbar_data& data,
        box<2,int> const& area, int axis, int content_size, int window_size,
        int position, int line_increment, int page_increment);

    void do_pass();

    int get_position() const { return logical_position; }

 private:
    void set_logical_position(int position);

    int get_max_logical_position() const;

    void set_physical_position(int position);

    widget_state get_button_state(widget_id id, box<2,int> const& area);
    widget_state get_thumb_state(widget_id id, box<2,int> const& area);

    void process_button_input(widget_id id, box<2,int> const& area,
        int increment);

    ui_context& ctx;
    scrollbar_data& data;
    bool enabled;
    box<2,int> area;
    int axis, content_size, window_size, logical_position, line_increment,
        page_increment, max_physical_position;
};

scrollbar::scrollbar(ui_context& ctx, scrollbar_data& data,
    box<2,int> const& area, int axis, int content_size, int window_size,
    int position, int line_increment, int page_increment)
  : ctx(ctx)
  , data(data)
  , area(area)
  , axis(axis)
  , content_size(content_size)
  , window_size(window_size)
  , logical_position(position)
  , line_increment(line_increment)
  , page_increment(page_increment)
{
}

void scrollbar::set_logical_position(int position)
{
    logical_position = clamp(position, 0, get_max_logical_position());

    data.physical_position = clamp(logical_position *
        max_physical_position / get_max_logical_position(), 0,
        max_physical_position);
}

int scrollbar::get_max_logical_position() const
{
    return content_size - window_size;
}

void scrollbar::set_physical_position(int position)
{
    data.physical_position = clamp(position, 0, max_physical_position);

    logical_position = max_physical_position <= 0 ? 0 :
        clamp(data.physical_position * get_max_logical_position() /
        max_physical_position, 0, get_max_logical_position());
}

widget_state scrollbar::get_button_state(widget_id id,
    box<2,int> const& area)
{
    if (detect_click_in_progress(ctx, id, LEFT_BUTTON))
    {
        return WIDGET_DEPRESSED;
    }
    else if (detect_potential_click(ctx, id))
    {
        return WIDGET_HOT;
    }
    else
        return WIDGET_NORMAL;
}

widget_state scrollbar::get_thumb_state(widget_id id,
    box<2,int> const& area)
{
    if (detect_drag_in_progress(ctx, id, LEFT_BUTTON))
    {
        return WIDGET_DEPRESSED;
    }
    else if (detect_potential_click(ctx, id))
    {
        return WIDGET_HOT;
    }
    else
        return WIDGET_NORMAL;
}

void scrollbar::process_button_input(widget_id id,
    box<2,int> const& area, int increment)
{
    static int const delay_after_first_increment = 400;
    static int const delay_after_other_increment = 40;

    if (detect_mouse_press(ctx, id, LEFT_BUTTON))
    {
        set_logical_position(logical_position + increment);
        start_timer(ctx, id, delay_after_first_increment);
    }
    else if (detect_click_in_progress(ctx, id, LEFT_BUTTON) &&
        is_timer_done(ctx, id))
    {
        set_logical_position(logical_position + increment);
        restart_timer(ctx, id, delay_after_other_increment);
    }
}

void scrollbar::do_pass()
{
    switch (ctx.event->category)
    {
     case RENDER_CATEGORY:
     case REGION_CATEGORY:
     case INPUT_CATEGORY:
        break;
     default:
        return;
    }

    // TODO: This should never actually happen, but it does.
    if (content_size <= 0 || window_size <= 0)
        return;

    if (window_size >= content_size)
    {
        enabled = false;
        return;
    }
    else
        enabled = true;

    assert(axis == 0 || axis == 1);
    int major_axis = axis;
    int minor_axis = 1 - axis;

    // If this data was previously used for a scrollbar on a different axis,
    // we need to clear out the cached rendering data.
    if (data.axis != major_axis)
    {
        data.rendering.data.reset();
        data.axis = major_axis;
    }

    scrollbar_renderer const* renderer = data.rendering.renderer;

    vector<2,int> button_size =
        make_vector<int>(renderer->width(ctx), renderer->button_length(ctx));

    box<2,int> button0_area = area;
    button0_area.size[major_axis] = button_size[1];
    widget_id button0_id = &data.button_id_data[0];

    box<2,int> button1_area = area;
    button1_area.corner[major_axis] =
        get_high_corner(area)[major_axis] - button_size[1];
    button1_area.size[major_axis] = button_size[1];
    widget_id button1_id = &data.button_id_data[1];

    box<2,int> full_bg_area = area;
    full_bg_area.corner[major_axis] += button_size[1];
    full_bg_area.size[major_axis] -= button_size[1] * 2;

    box<2,int> thumb_area = full_bg_area;
    thumb_area.size[major_axis] = (std::max)(
        renderer->minimum_thumb_length(ctx),
        window_size * full_bg_area.size[major_axis] / content_size);

    max_physical_position = full_bg_area.size[major_axis] -
        thumb_area.size[major_axis];

    if (max_physical_position < 0)
    {
        if (ctx.event->type == RENDER_EVENT && area.size[0] > 0 &&
            area.size[1] > 0)
        {
            data.rendering.renderer->draw_background(
                ctx, data.rendering.data, area, axis, 0, WIDGET_NORMAL);
        }
        return;
    }

    // Check that the physical position is consistent with the logical
    // position.  If not, it's set accordingly.  The physical position is
    // only there to provide extra precision.
    int position = logical_position;
    set_physical_position(data.physical_position);
    if (position != logical_position)
        set_logical_position(position);

    thumb_area.corner[major_axis] = full_bg_area.corner[major_axis] +
        data.physical_position;
    widget_id thumb_id = &data.thumb_id_data;

    int thumb_center = data.physical_position +
        thumb_area.size[major_axis] / 2;

    box<2,int> bg0_area = full_bg_area;
    bg0_area.size[major_axis] = thumb_area.corner[major_axis] -
        full_bg_area.corner[major_axis];
    widget_id bg0_id = &data.background_id_data[0];

    box<2,int> bg1_area = full_bg_area;
    bg1_area.corner[major_axis] = get_high_corner(thumb_area)[major_axis];
    bg1_area.size[major_axis] = get_high_corner(full_bg_area)[major_axis] -
        bg1_area.corner[major_axis];
    widget_id bg1_id = &data.background_id_data[1];

    switch (ctx.event->category)
    {
     case RENDER_CATEGORY:
      {
        if (bg0_area.size[major_axis] != 0)
        {
            data.rendering.renderer->draw_background(
                ctx, data.rendering.data, bg0_area, axis, 0,
                get_button_state(bg0_id, bg0_area));
        }
        if (bg1_area.size[major_axis] != 0)
        {
            data.rendering.renderer->draw_background(
                ctx, data.rendering.data, bg1_area, axis, 1,
                get_button_state(bg1_id, bg1_area));
        }
        data.rendering.renderer->draw_thumb(
            ctx, data.rendering.data, thumb_area, axis,
            get_thumb_state(thumb_id, thumb_area));
        data.rendering.renderer->draw_button(
            ctx, data.rendering.data, button0_area.corner, axis, 0,
            get_button_state(button0_id, button0_area));
        data.rendering.renderer->draw_button(
            ctx, data.rendering.data, button1_area.corner, axis, 1,
            get_button_state(button1_id, button1_area));
        break;
      }

     case REGION_CATEGORY:
      {
        do_box_region(ctx, bg0_id, bg0_area);
        do_box_region(ctx, bg1_id, bg1_area);
        do_box_region(ctx, thumb_id, thumb_area);
        do_box_region(ctx, button0_id, button0_area);
        do_box_region(ctx, button1_id, button1_area);
        break;
      }

     case INPUT_CATEGORY:
      {
        process_button_input(bg0_id, bg0_area, -page_increment);
        process_button_input(bg1_id, bg1_area, page_increment);

        if (detect_mouse_press(ctx, thumb_id, LEFT_BUTTON))
        {
            data.drag_start_delta = data.physical_position -
                get_integer_mouse_position(ctx)[major_axis];
        }
        if (detect_drag(ctx, thumb_id, LEFT_BUTTON))
        {
            set_physical_position(get_integer_mouse_position(ctx)[major_axis] +
                data.drag_start_delta);
        }

        process_button_input(button0_id, button0_area, -line_increment);
        process_button_input(button1_id, button1_area, line_increment);

        float movement;
        if (detect_wheel_movement(ctx, &movement) &&
            mouse_is_inside_box(ctx, box<2,double>(area)))
        {
            set_logical_position(
                int(logical_position - movement * line_increment + 0.5));
            acknowledge_input_event(ctx);
        }
      }
    }
}

int do_scrollbar(ui_context& ctx, scrollbar_data& data,
    box<2,int> const& area, int axis, int content_size, int window_size,
    int position, int line_increment, int page_increment)
{
    if (page_increment < 0)
    {
        page_increment = (std::max)(line_increment,
            window_size - line_increment);
    }
    scrollbar sb(ctx, data, area, axis, content_size,
        window_size, position, line_increment, page_increment);
    sb.do_pass();
    return sb.get_position();
}

struct scrollbar_pair_data
{
    scrollbar_data sb_data[2];
    themed_rendering_data<scrollbar_junction_renderer> junction;
};

struct scrollable_layout_container : layout_container
{
    // implementation of layout interface
    layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx);
    layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

    // the actual state of the scrollable region
    layout_vector scroll_position;

    // layout cacher
    layout_cacher cacher;

    // set by caller and copied here
    unsigned scrollable_axes;

    // determined at usage site and needed by layout
    layout_scalar scrollbar_width, minimum_window_size, line_size;

    // determined by layout and stored here to communicate back to non-layout
    // passes
    bool hsb_on, vsb_on;
    layout_vector content_size, window_size;

    // data for scrollbars
    scrollbar_data hsb_data, vsb_data;

    // rendering data for junction
    themed_rendering_data<scrollbar_junction_renderer> junction_rendering;
};

layout_requirements scrollable_layout_container::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    horizontal_layout_query query(ctx, cacher, last_content_change);
    alia_if (query.update_required())
    {
        alia_if ((scrollable_axes & 1) != 0 && !ctx.for_measurement)
        {
            // If the window is horizontally scrollable, then we only need
            // enough space for scrolling to happen.
            query.update(
                calculated_layout_requirements(minimum_window_size, 0, 0));
        }
        alia_else
        {
            // Otherwise, we need to calculate the requirements.
            assert(children && !children->next); // one and only one child
            layout_requirements r =
                alia::get_horizontal_requirements(ctx, *children);
            layout_scalar required_width = r.minimum_size;
            if ((scrollable_axes & 2) != 0)
                required_width += scrollbar_width;
            query.update(
                calculated_layout_requirements(required_width, 0, 0));
        }
        alia_end
    }
    alia_end
    return query.result();
}

layout_requirements scrollable_layout_container::get_vertical_requirements(
    layout_calculation_context& ctx, layout_scalar assigned_width)
{
    vertical_layout_query query(ctx, cacher, last_content_change,
        assigned_width);
    alia_if (query.update_required())
    {
        alia_if ((scrollable_axes & 2) != 0 && !ctx.for_measurement)
        {
            // If the window is vertically scrollable, then we only need
            // enough space for scrolling to happen.
            query.update(
                calculated_layout_requirements(minimum_window_size, 0, 0));
        }
        alia_else
        {
            // Otherwise, we need to calculate the requirements.
            assert(children && !children->next); // one and only one child
            layout_scalar resolved_width =
                resolve_assigned_width(
                    this->cacher.resolved_spec,
                    assigned_width,
                    this->get_horizontal_requirements(ctx));
            layout_requirements x =
                alia::get_horizontal_requirements(ctx, *children);
            layout_scalar actual_width =
                (std::max)(resolved_width, x.minimum_size);
            layout_requirements y = alia::get_vertical_requirements(
                ctx, *children, actual_width);
            layout_scalar required_height = y.minimum_size;
            if ((scrollable_axes & 1) != 0)
                required_height += scrollbar_width;
            query.update(
                calculated_layout_requirements(required_height, 0, 0));
        }
        alia_end
    }
    alia_end
    return query.result();
}

static void clamp_scroll_position(scrollable_layout_container& container)
{
    if (container.hsb_on)
    {
        container.scroll_position[0] = clamp(container.scroll_position[0], 0,
            container.content_size[0] - container.window_size[0]);
    }
    else
        container.scroll_position[0] = 0;
        
    if (container.vsb_on)
    {
        container.scroll_position[1] = clamp(container.scroll_position[1], 0,
            container.content_size[1] - container.window_size[1]);
    }
    else
        container.scroll_position[1] = 0;
}

void scrollable_layout_container::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    relative_region_assignment rra(ctx, *this, cacher, last_content_change,
        assignment);
    alia_if (rra.update_required())
    {
        layout_vector available_size = rra.resolved_assignment().region.size;

        assert(children && !children->next); // one and only one child
        layout_requirements x =
            alia::get_horizontal_requirements(ctx, *children);
        if (available_size[0] < x.minimum_size)
        {
            hsb_on = true;
            available_size[1] -= scrollbar_width;
        }
        else
            hsb_on = false;

        layout_requirements y = alia::get_vertical_requirements(
            ctx, *children, (std::max)(available_size[0], x.minimum_size));
        if (available_size[1] < y.minimum_size)
        {
            vsb_on = true;
            available_size[0] -= scrollbar_width;
            if (!hsb_on && available_size[0] < x.minimum_size)
            {
                hsb_on = true;
                available_size[1] -= scrollbar_width;
            }
        }
        else
            vsb_on = false;

        layout_scalar content_width =
            (std::max)(available_size[0], x.minimum_size);

        y = alia::get_vertical_requirements(ctx, *children, content_width);

        layout_scalar content_height =
            (std::max)(available_size[1], y.minimum_size);

        layout_vector content_size =
            make_layout_vector(content_width, content_height);

        // If the panel is scrolled all the way to the end, and the content
        // grows, scroll to show the new content.
        for (unsigned i = 0; i != 2; ++i)
        {
            if (scroll_position[i] != 0 &&
                scroll_position[i] + this->window_size[i] >=
                    this->content_size[i])
            {
                scroll_position[i] = content_size[i] - available_size[i];
            }
        }

        this->content_size = content_size;
        this->window_size = available_size;

        relative_layout_assignment assignment(
            layout_box(make_layout_vector(0, 0), content_size),
            content_height - y.minimum_descent);

        clamp_scroll_position(*this);

        alia::set_relative_assignment(ctx, *children, assignment);
        rra.update();
    }
    alia_end
}

void scrollable_region::begin(
    ui_context& ctx,
    layout const& layout_spec,
    unsigned scrollable_axes,
    widget_id id)
{
    ctx_ = &ctx;
    id_ = id;

    scrollable_layout_container* container;
    if (get_data(ctx, &container))
        container->scroll_position = make_vector<int>(0, 0);
    container_ = container;

    slc_.begin(get_layout_traversal(ctx), container);

    srr_.begin(ctx.routing);

    if (is_refresh_pass(ctx))
    {
        detect_layout_change(get_layout_traversal(ctx),
            &container->scrollable_axes, scrollable_axes);

        update(get_layout_traversal(ctx), container->cacher, layout_spec,
            FILL | UNPADDED);

        static default_scrollbar_junction_renderer default_renderer;
        refresh_themed_rendering_data(ctx, container->junction_rendering,
            &default_renderer);

        refresh_scrollbar_data(ctx, container->hsb_data);
        refresh_scrollbar_data(ctx, container->vsb_data);

        detect_layout_change(ctx, &container->scrollbar_width,
            get_scrollbar_width(ctx, container->vsb_data));
        detect_layout_change(ctx, &container->minimum_window_size,
            get_minimum_scrollbar_length(ctx, container->vsb_data));
        container->line_size = resolve_layout_width(ctx.layout, 10, EM);
    }
    else
    {
        layout_vector window_corner =
            container->cacher.resolved_relative_assignment.region.corner;

        if (container->hsb_on)
        {
            container->scroll_position[0] = do_scrollbar(
                ctx, container->hsb_data,
                box<2,int>(
                    window_corner +
                    make_vector<int>(0, container->window_size[1]),
                    make_vector<int>(container->window_size[0],
                        container->scrollbar_width)),
                0, container->content_size[0], container->window_size[0],
                container->scroll_position[0], container->line_size,
                container->window_size[0]);
        }
        if (container->vsb_on)
        {
            container->scroll_position[1] = do_scrollbar(
                ctx, container->vsb_data,
                box<2,int>(
                    window_corner +
                    make_vector<int>(container->window_size[0], 0),
                    make_vector<int>(container->scrollbar_width,
                        container->window_size[1])),
                1, container->content_size[1], container->window_size[1],
                container->scroll_position[1], container->line_size,
                container->window_size[1]);
        }
        if (container->hsb_on && container->vsb_on)
        {
            if (is_rendering(ctx))
            {
                container->junction_rendering.renderer->draw(
                    ctx, container->junction_rendering.data,
                    box<2,int>(window_corner, container->window_size));
            }
        }

        scr_.begin(*get_layout_traversal(ctx).geometry);
        scr_.set(box<2,double>(vector<2,double>(window_corner),
            vector<2,double>(container->window_size)));

        transform_.begin(*get_layout_traversal(ctx).geometry);
        transform_.set(translation_matrix(vector<2,double>(
            window_corner - container->scroll_position)));
    }
}
void scrollable_region::end()
{
    if (!ctx_)
        return;
    ui_context& ctx = *ctx_;

    switch (ctx.event->category)
    {
     case REGION_CATEGORY:
        if (ctx.event->type == MAKE_WIDGET_VISIBLE_EVENT &&
            srr_.is_relevant())
        {
            make_widget_visible_event& e =
                get_event<make_widget_visible_event>(ctx);
            if (e.acknowledged)
            {
                // TODO: This doesn't handle rotations properly.
                vector<2,double>
                    region_ul =
                        transform(
                            inverse(ctx.geometry.transformation_matrix),
                            e.region.corner),
                    region_lr =
                        transform(
                            inverse(ctx.geometry.transformation_matrix),
                            get_high_corner(e.region)),
                    window_ul =
                        vector<2,double>(container_->scroll_position),
                    window_lr =
                        window_ul + vector<2,double>(container_->window_size);
                for (int i = 0; i < 2; ++i)
                {
                    if (e.region.size[i] <= double(container_->window_size[i]))
                    {
                        if (region_ul[i] < window_ul[i] &&
                            region_lr[i] < window_lr[i])
                        {
                            int correction =
                                int(window_ul[i] - region_ul[i] + 0.5);
                            container_->scroll_position[i] -= correction;
                            e.region.corner[i] += correction;
                        }
                        else if (region_ul[i] > window_ul[i] &&
                            region_lr[i] > window_lr[i])
                        {
                            int correction =
                                int((std::min)(region_ul[i] - window_ul[i],
                                    region_lr[i] - window_lr[i]) + 0.5);
                            container_->scroll_position[i] += correction;
                            e.region.corner[i] -= correction;
                        }
                    }
                    else
                    {
                        if (region_lr[i] < window_ul[i] ||
                            region_ul[i] >= window_lr[i])
                        {
                            int correction =
                                int(window_ul[i] - region_ul[i] + 0.5);
                            container_->scroll_position[i] -= correction;
                            e.region.corner[i] += correction;
                        }
                    }
                }
            }
        }
        break;

     case INPUT_CATEGORY:
        if (srr_.is_relevant() || id_has_focus(ctx, id_))
        {
            key_event_info info;
            if (detect_key_press(ctx, &info) && info.mods == 0)
            {
                switch (info.code)
                {
                 case KEY_UP:
                    container_->scroll_position[1] -= container_->line_size;
                    break;
                 case KEY_DOWN:
                    container_->scroll_position[1] += container_->line_size;
                    break;
                 case KEY_PAGEUP:
                    container_->scroll_position[1] -= (std::max)(
                        container_->window_size[1] - container_->line_size,
                        container_->line_size);
                    break;
                 case KEY_PAGEDOWN:
                    container_->scroll_position[1] += (std::max)
                        (container_->window_size[1] - container_->line_size,
                        container_->line_size);
                    break;
                 case KEY_LEFT:
                    container_->scroll_position[0] -= container_->line_size;
                    break;
                 case KEY_RIGHT:
                    container_->scroll_position[0] += container_->line_size;
                    break;
                 case KEY_HOME:
                    container_->scroll_position[1] = 0;
                    break;
                 case KEY_END:
                    container_->scroll_position[1] =
                        container_->content_size[1] -
                        container_->window_size[1];
                    break;
                }
                clamp_scroll_position(*container_);
            }
        }
        break;
    }

    ctx_ = 0;

    transform_.end();
    scr_.end();

    float movement;
    if (detect_wheel_movement(ctx, &movement) &&
        mouse_is_inside_box(ctx,
            box<2,double>(vector<2,double>(
                container_->cacher.resolved_relative_assignment.region.
                    corner),
                vector<2,double>(container_->window_size))))
    {
        container_->scroll_position[1] -=
            int(container_->line_size * movement);
        clamp_scroll_position(*container_);
        acknowledge_input_event(ctx);
    }

    srr_.end();
    slc_.end();
}

}
