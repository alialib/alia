#include <alia/canvas.hpp>
#include <alia/context.hpp>
#include <alia/layout.hpp>
#include <alia/input_utils.hpp>
#include <alia/transformations.hpp>
#include <alia/input_events.hpp>
#include <alia/drawing.hpp>
#include <alia/image.hpp>
#include <alia/spacer.hpp>
#include <boost/format.hpp>

namespace alia {

double zoom_to_fit_scene(vector2i const& canvas_size,
    vector2d const& scene_size)
{
    return (std::min)(canvas_size[0] / scene_size[0],
        canvas_size[1] / scene_size[1]);
}
double zoom_to_fit_scene_height(vector2i const& canvas_size,
    vector2d const& scene_size)
{
    return canvas_size[1] / scene_size[1];
}
double zoom_to_fit_scene_width(vector2i const& canvas_size,
    vector2d const& scene_size)
{
    return canvas_size[0] / scene_size[0];
}
double zoom_to_fill_canvas(vector2i const& canvas_size,
    vector2d const& scene_size)
{
    return (std::max)(canvas_size[0] / scene_size[0],
        canvas_size[1] / scene_size[1]);
}

template<typename T>
T clamp(T x, T min, T max)
{
    assert(min <= max);
    return (std::min)((std::max)(x, min), max);
}

void default_camera::set_position(canvas const& canvas, point2d const& p)
{
    position_ = p;
    check_bounds(canvas);
}
point2d default_camera::get_position(canvas const& canvas) const
{
    return position_;
}
double default_camera::get_zoom_level(canvas const& canvas) const
{
    double zoom = zoom_.evaluate(canvas.region().size,
        canvas.scene_box().size);
    double min_zoom = get_min_zoom(canvas);
    if (min_zoom != 0. && zoom < min_zoom)
        zoom = min_zoom;
    double max_zoom = get_max_zoom(canvas);
    if (max_zoom != 0. && zoom > max_zoom)
        zoom = max_zoom;
    return zoom;
}
void default_camera::set_zoom_level(canvas const& canvas, zoom_level zoom)
{
    zoom_ = zoom;
    check_bounds(canvas);
}
double default_camera::get_min_zoom(canvas const& canvas) const
{
    return min_zoom_.evaluate(canvas.region().size,
        canvas.scene_box().size);
}
double default_camera::get_max_zoom(canvas const& canvas) const
{
    return max_zoom_.evaluate(canvas.region().size,
        canvas.scene_box().size);
}
void default_camera::check_bounds(canvas const& canvas)
{
    box2d const& sb = canvas.scene_box();
    for (int i = 0; i < 2; ++i)
    {
        double margin = constrained_ ? double(canvas.region().size[i]) /
            2 / get_zoom_level(canvas) : 0;
        if (margin <= sb.size[i] / 2)
        {
            position_[i] = clamp(position_[i], sb.corner[i] + margin,
                get_high_corner(sb)[i] - margin);
        }
        else
            position_[i] = get_center(sb)[i];
    }
}

struct canvas::data
{
    alia::layout_data layout_data;
};

void canvas::begin(alia::context& ctx, box2d const& scene_box,
    alia::camera* camera, flag_set flags, layout const& layout_spec)
{
    ctx_ = &ctx;
    data_ = get_data<data>(ctx);
    id_ = get_region_id(ctx);
    scene_box_ = scene_box;
    flags_ = flags;
    if (!camera)
    {
        default_camera* dc;
        if (get_data(ctx, &dc))
            dc->initialize(get_center(scene_box), zoom_level());
    }
    else
        camera_ = camera;

    if (ctx.event->category == LAYOUT_CATEGORY)
    {
        // TODO: What should the default layout behavior be?
        layout_widget(ctx, data_->layout_data, layout_spec,
            resolve_size(ctx, layout_spec.size),
            widget_layout_info(vector2i(0, 0), 0, 0, vector2i(0, 0),
                GROW, true));
    }

    do_region(ctx, id_, region());

    scr_.begin(ctx);
    scr_.set(data_->layout_data.assigned_region);

    st_.begin(ctx);
    set_scene_coordinates();

    active_ = true;
}

void canvas::end()
{
    if (active_)
    {
        st_.end();
        scr_.end();
        active_ = false;
    }
}

box2i const& canvas::region() const
{
    return data_->layout_data.assigned_region;
}

void canvas::set_scene_coordinates()
{
    double scale_factor = get_zoom_level();
    vector2d scale_vector(
        (flags_ & FLIP_X) ? -scale_factor : scale_factor,
        (flags_ & FLIP_Y) ? -scale_factor : scale_factor);
    st_.set(
        translation<2,double>(
            vector2d(region().corner) +
            vector2d(region().size) / 2) *
        scaling_transformation(scale_vector) *
        translation(-vector2d(get_camera_position())));
}

void canvas::set_canvas_coordinates()
{
    st_.restore();
}

point2d canvas_to_scene(canvas& c, point2d const& p, double zoom_level,
    point2d const& camera_position)
{
    double zoom = 1. / zoom_level;
    vector2d scale_factor(
        c.flip_x() ? -zoom : zoom,
        c.flip_y() ? -zoom : zoom);
    return point2d(
        (p - (point2d(c.region().corner) +
        vector2d(c.region().size) / 2)) * scale_factor +
        camera_position);
}
point2d scene_to_canvas(canvas& c, point2d const& p, double zoom_level,
    point2d const& camera_position)
{
    double zoom = zoom_level;
    vector2d scale_factor(
        c.flip_x() ? -zoom : zoom,
        c.flip_y() ? -zoom : zoom);
    return point2d(
        (p - camera_position) * scale_factor +
        (point2d(c.region().corner) +
        vector2d(c.region().size) / 2));
}
point2d canvas_to_scene(canvas& c, point2d const& p)
{
    return canvas_to_scene(c, p, c.get_zoom_level(), c.get_camera_position());
}
point2d scene_to_canvas(canvas& c, point2d const& p)
{
    return scene_to_canvas(c, p, c.get_zoom_level(), c.get_camera_position());
}

void apply_panning_tool(canvas& canvas, mouse_button button)
{
    context& ctx = canvas.context();
    if (ctx.event->type == RENDER_EVENT &&
        detect_drag_in_progress(ctx, canvas.id(), button))
    {
        ctx.surface->set_mouse_cursor(FOUR_WAY_ARROW_CURSOR);
    }
    if (detect_drag(ctx, canvas.id(), button))
    {
        mouse_motion_event& e = get_event<mouse_motion_event>(ctx);
        canvas.set_camera_position(canvas.get_camera_position() -
            get_drag_delta(ctx, e));
    }
}

void draw_checker_background(canvas& canvas, rgb8 const& color1,
    rgb8 const& color2, double spacing)
{
    // TODO: This won't handle changes in the color arguments.
    image<rgb8>* img;
    if (get_data(canvas.context(), &img))
    {
        create_image(*img, vector2i(2, 2));
        img->view.pixels[0] = img->view.pixels[3] = color1;
        img->view.pixels[1] = img->view.pixels[2] = color2;
    }
    point2d p;
    box2d region;
    scoped_transformation st;
    if (canvas.context().event->type == RENDER_EVENT)
    {
        point2d const corner0 = canvas_to_scene(canvas,
            point2d(canvas.region().corner));
        point2d const corner1 = canvas_to_scene(canvas,
            point2d(get_high_corner(canvas.region())));
        for (int i = 0; i < 2; ++i)
        {
            p[i] = std::floor(std::min(corner0[i], corner1[i]) / spacing);
            region.corner[i] = p[i];
            region.size[i] = std::ceil(std::abs((corner1 - corner0)[i]) /
                spacing + 1);
        }
        st.begin(canvas.context());
        st.set(scaling_transformation(vector2d(spacing, spacing)));
    }
    draw_image_region(
        canvas.context(),
        p,
        make_interface(img->view, 0),
        region,
        rgba8(0xff, 0xff, 0xff, 0xff),
        surface::TILED_IMAGE);
}

static void draw_grid_lines_for_axis(canvas& canvas, box2d const& box,
    rgba8 const& color, line_style const& style, double spacing,
    unsigned axis, unsigned skip)
{
    // TODO: skipping
    double start = std::ceil(box.corner[axis] / spacing) * spacing,
        end = get_high_corner(box)[axis];
    unsigned other_axis = 1 - axis;
    point2d p0, p1;
    p0[other_axis] = box.corner[other_axis];
    p0[axis] = start;
    p1[other_axis] = get_high_corner(box)[other_axis];
    p1[axis] = start;
    surface& surface = *canvas.context().surface;
    while (p0[axis] <= end)
    {
        surface.draw_line(color, style, p0, p1);
        p0[axis] += spacing;
        p1[axis] += spacing;
    }
}

void draw_grid_lines(canvas& canvas, box2d const& box, rgba8 const& color,
    line_style const& style, double spacing, unsigned axis, unsigned skip)
{
    if (canvas.context().event->type == RENDER_EVENT)
    {
        // TODO: axis selection
        draw_grid_lines_for_axis(canvas, box, color, style, spacing, 0, skip);
        draw_grid_lines_for_axis(canvas, box, color, style, spacing, 1, skip);
    }
}

static void draw_rotated_ascii_text(
    surface& surface, point2d const& p, double angle,
    rgba8 const& color, alia::font const& font, char const* text)
{
    scoped_surface_transformation st(surface,
        translation(vector2d(p)) * rotation(angle));
    surface.draw_ascii_text(point2d(0, 0), color, font, text);
}

static void label_ruler(
    context& ctx, int iterations,
    double initial_value, double value_increment,
    point2d const& initial_location, vector2d const& location_increment,
    bool draw_tenth_ticks, bool label_half_ticks,
    vector2d const& full_tick_offset, vector2d const& half_tick_offset,
    vector2d const& tenth_tick_offset, double text_rotation_angle,
    vector2d const& text_offset, bool draw_mouse,
    point2d const& mouse_position, vector2d const& mouse_offset,
    vector2d const& mouse_lateral_offset,
    rgba8 const& color, alia::font const& font)
{
    surface& surface = *ctx.surface;

    point2d location = initial_location;
    double value = initial_value;

    double minor_value_inc = value_increment * .1;
    vector2d minor_location_inc = location_increment * .1;

    for (int i = 0; i < iterations; ++i)
    {
        surface.draw_line(color, line_style(1, solid_line),
            location, point2d(location + full_tick_offset));
        value = std::floor(value / minor_value_inc + 0.5) * minor_value_inc;
        if (value == 0) value = 0; // eliminate -0's
        draw_rotated_ascii_text(
            surface, point2d(location + text_offset), text_rotation_angle,
            color, font, str(boost::format("%s") % value).c_str());
        location += minor_location_inc;
        value += minor_value_inc;

        if (draw_tenth_ticks)
        {
            for (int i = 0; i < 4; ++i)
            {
                surface.draw_line(color, line_style(1, solid_line), location,
                    point2d(location + tenth_tick_offset));
                location += minor_location_inc;
                value += minor_value_inc;
            }
        }
        else
        {
            location += minor_location_inc * 4;
            value += minor_value_inc * 4;
        }

        surface.draw_line(color, line_style(1, solid_line),
            location, point2d(location + half_tick_offset));
        if (label_half_ticks)
        {
            value = std::floor(value / minor_value_inc + 0.5) *
                minor_value_inc;
            draw_rotated_ascii_text(
                surface, point2d(location + text_offset), text_rotation_angle,
                color, font, str(boost::format("%s") % value).c_str());
        }
        location += minor_location_inc;
        value += minor_value_inc;

        if (draw_tenth_ticks)
        {
            for (int i = 0; i < 4; ++i)
            {
                surface.draw_line(color, line_style(1, solid_line), location,
                    point2d(location + tenth_tick_offset));
                location += minor_location_inc;
                value += minor_value_inc;
            }
        }
        else
        {
            location += minor_location_inc * 4;
            value += minor_value_inc * 4;
        }
    }

    if (draw_mouse)
    {
        point2d poly[3];
        poly[0] = mouse_position;
        poly[1] = mouse_position + mouse_offset + mouse_lateral_offset;
        poly[2] = mouse_position + mouse_offset - mouse_lateral_offset;
        surface.draw_filled_polygon(color, poly, 3);
    }
}

// Calculate the location and values of the ruler marks and store the
// information in the provided variables.
static void calculate_ruler_values(
    canvas& canvas,
    box2i const& region,
    point2d& initial_value, vector2d& value_inc,
    point2d& initial_location, vector2d& location_inc,
    vector2i& n_major_ticks)
{
    // NOTE: "ruler space" refers to the current coordinate frame of the
    // canvas.  The coordinates labeled on the rulers refer to this space.

    // the location of some key points on the canvas, transformed into
    // ruler space
    point2d canvas_origin = canvas_to_scene(canvas, point2d(region.corner)),
        canvas_x = canvas_to_scene(canvas,
            point2d(region.corner + vector2i(1, 0))),
        canvas_y = canvas_to_scene(canvas,
            point2d(region.corner + vector2i(0, 1)));

    // get unit vectors describing the canvas axes in ruler space
    vector2d dx = unit(canvas_x - canvas_origin),
        dy = unit(canvas_y - canvas_origin);

    // take the absolute values of the vector components
    dx[0] = std::abs(dx[0]);
    dx[1] = std::abs(dx[1]);
    dy[0] = std::abs(dy[0]);
    dy[1] = std::abs(dy[1]);

    // NOTE: The word "value" below refers to the numerical values on the
    // rulers (i.e, the location in ruler space).  "location" refers to the
    // position on the canvas.

    point2d value_at_origin(dot(vector2d(canvas_origin), dx),
        dot(vector2d(canvas_origin), dy));

    vector2d value_inc_per_pixel(
        point2d(dot(vector2d(canvas_x), dx), dot(vector2d(canvas_y), dy)) -
        value_at_origin);

    // TODO: This should use physicals units, or units relative to the font
    // size, not pixels.

    vector2d values_per_pixel(std::abs(value_inc_per_pixel[0]),
        std::abs(value_inc_per_pixel[1]));

    // determine the major tick spacing (in values)
    vector2d major_tick_spacing;
    for (int i = 0; i < 2; ++i)
    {
        major_tick_spacing[i] = 1000000;
        while (major_tick_spacing[i] / values_per_pixel[i] > 700)
            major_tick_spacing[i] /= 10;
    }

    // determine the value increment per major tick
    for (int i = 0; i < 2; ++i)
    {
        value_inc[i] = value_inc_per_pixel[i] > 0 ? major_tick_spacing[i] :
            -major_tick_spacing[i];
    }

    // determine the value of the first tick (which will be safely off
    // screen)
    for (int i = 0; i < 2; ++i)
    {
        initial_value[i] = floor(value_at_origin[i] / value_inc[i]) *
            value_inc[i];
    }

    // determine the canvas location of the first tick
    initial_location = point2d(region.corner) +
        (initial_value - value_at_origin) / value_inc_per_pixel;

    location_inc = value_inc / value_inc_per_pixel;

    // determine how many major ticks are needed along each axis
    for (int i = 0; i < 2; ++i)
    {
        n_major_ticks[i] = static_cast<int>(region.size[i] /
            std::abs(location_inc[i])) + 3;
    }
}

int get_ruler_width(context& ctx, flag_set flags)
{
    bool draw_border = (flags & DRAW_BORDER) != 0;
    alia::font const& font = ctx.pass_state.active_font;
    int text_height = get_font_metrics(ctx, font).height;
    int ruler_width = text_height * 5 / 4;
    if (!draw_border)
        --ruler_width;
    return ruler_width;
}

void draw_side_ruler(
    canvas& canvas, box2i const& region,
    rgba8 const& bg_color, rgba8 const& fg_color,
    double scale, flag_set flags)
{
    // TODO: Clean this up. Lots of stuff applies to both axes, but the
    // function only handles one axis at a time.

    context& ctx = canvas.context();
    if (ctx.event->type != RENDER_EVENT)
        return;

    bool mouse_inside = mouse_is_inside_region(ctx, canvas.region());

    scoped_transformation st;
    st.begin(ctx);
    canvas.set_canvas_coordinates();
    scoped_clip_region scr;
    scr.begin(ctx);

    bool draw_border = (flags & DRAW_BORDER) != 0;

    alia::font const& font = ctx.pass_state.active_font;

    int text_height = get_font_metrics(ctx, font).height;

    point2d initial_value, initial_location;
    vector2d value_inc, location_inc;
    vector2i n_major_ticks;
    calculate_ruler_values(canvas, canvas.region(), initial_value,
        value_inc, initial_location, location_inc, n_major_ticks);

    initial_value[0] /= scale;
    initial_value[1] /= scale;
    value_inc /= scale;

    // TODO: These shouldn't simply be hardcoded.
    bool label_half_ticks[2], draw_tenth_ticks[2];
    for (int i = 0; i < 2; ++i)
    {
        label_half_ticks[i] = std::abs(location_inc[i]) > 180;
        draw_tenth_ticks[i] = std::abs(location_inc[i]) > 120;
    }

    // draw the ruler

    scr.set(region);

    draw_filled_box(ctx, bg_color, region);

    double full_tick = text_height * .75,
        half_tick = text_height * .5,
        minor_tick = text_height * .25,
        mouse_spacing = half_tick + 2,
        mouse_arrow = 6;

    box2i r = region;

    switch (flags.code & SIDE_MASK_CODE)
    {
     case BOTTOM_SIDE_CODE:
      {
        int border_y;
        if (draw_border)
        {
            border_y = r.corner[1]++;
            --r.size[1];
        }
        label_ruler(
            ctx, n_major_ticks[0], initial_value[0], value_inc[0],
            point2d(initial_location[0], r.corner[1]),
            vector2d(location_inc[0], 0),
            draw_tenth_ticks[0], label_half_ticks[0],
            vector2d(0, full_tick), vector2d(0, half_tick),
            vector2d(0, minor_tick),
            0, vector2d(minor_tick - 1, minor_tick - 1),
            mouse_inside,
            point2d(ctx.pass_state.mouse_position[0],
                r.corner[1] + mouse_spacing),
            vector2d(0, mouse_arrow), vector2d(mouse_arrow, 0),
            fg_color, font);
        if (draw_border)
        {
            ctx.surface->draw_line(fg_color, line_style(1, solid_line),
                point2d(r.corner[0], border_y + 0.375),
                point2d(get_high_corner(r)[0], border_y + 0.375));
        }
        break;
      }
     case TOP_SIDE_CODE:
      {
        int border_y;
        if (draw_border)
        {
            border_y = get_high_corner(r)[1] - 1;
            --r.size[1];
        }
        label_ruler(
            ctx, n_major_ticks[0], initial_value[0], value_inc[0],
            point2d(initial_location[0], (r.corner + r.size)[1]),
            vector2d(location_inc[0], 0),
            draw_tenth_ticks[0], label_half_ticks[0],
            vector2d(0, -full_tick), vector2d(0, -half_tick),
            vector2d(0, -minor_tick),
            0, vector2d(minor_tick - 1, -text_height - (minor_tick - 1)),
            mouse_inside,
            point2d(ctx.pass_state.mouse_position[0],
                (r.corner + r.size)[1] - mouse_spacing),
            vector2d(0, -mouse_arrow), vector2d(mouse_arrow, 0),
            fg_color, font);
        if (draw_border)
        {
            ctx.surface->draw_line(fg_color, line_style(1, solid_line),
                point2d(r.corner[0], border_y + 0.375),
                point2d(get_high_corner(r)[0], border_y + 0.375));
        }
        break;
      }
     case RIGHT_SIDE_CODE:
      {
        int border_x;
        if (draw_border)
        {
            border_x = r.corner[0]++;
            --r.size[0];
        }
        label_ruler(
            ctx, n_major_ticks[1], initial_value[1], value_inc[1],
            point2d(r.corner[0], initial_location[1]),
            vector2d(0, location_inc[1]),
            draw_tenth_ticks[1], label_half_ticks[1],
            vector2d(full_tick, 0), vector2d(half_tick, 0),
            vector2d(minor_tick, 0),
            pi / 2,
            vector2d(text_height + (minor_tick - 1), minor_tick - 1),
            mouse_inside,
            point2d(r.corner[0] + mouse_spacing,
                ctx.pass_state.mouse_position[1]),
            vector2d(mouse_arrow, 0), vector2d(0, mouse_arrow),
            fg_color, font);
        if (draw_border)
        {
            ctx.surface->draw_line(fg_color, line_style(1, solid_line),
                point2d(border_x + 0.375, r.corner[1]),
                point2d(border_x + 0.375, get_high_corner(r)[1]));
        }
      }
     case LEFT_SIDE_CODE:
      {
        int border_x;
        if (draw_border)
        {
            border_x = get_high_corner(r)[0] - 1;
            --r.size[0];
        }
        label_ruler(
            ctx, n_major_ticks[1], initial_value[1], value_inc[1],
            point2d((r.corner + r.size)[0], initial_location[1]),
            vector2d(0, location_inc[1]),
            draw_tenth_ticks[1], label_half_ticks[1],
            vector2d(-full_tick, 0), vector2d(-half_tick, 0),
            vector2d(-minor_tick, 0),
            -pi / 2,
            vector2d(-text_height - (minor_tick - 1), -(minor_tick - 1)),
            mouse_inside,
            point2d((r.corner + r.size)[0] - mouse_spacing,
                ctx.pass_state.mouse_position[1]),
            vector2d(-mouse_arrow, 0), vector2d(0, mouse_arrow),
            fg_color, font);
        if (draw_border)
        {
            ctx.surface->draw_line(fg_color, line_style(1, solid_line),
                point2d(border_x + 0.375, r.corner[1]),
                point2d(border_x + 0.375, get_high_corner(r)[1]));
        }
      }
    }
}

void side_rulers::begin(context& ctx, flag_set flags,
    layout const& layout_spec)
{
    ctx_ = &ctx;
    flags_ = flags;
    scales_[0] = scales_[1] = 1;
    active_ = true;
    canvas_ = 0;
    grid_.begin(ctx, layout_spec);
    do_ruler_row(TOP_SIDE);
    row_.begin(grid_, GROW);
    if ((flags & LEFT_SIDE) != 0)
        reserve_space(LEFT_SIDE, 0);
}
void side_rulers::set_canvas(canvas& canvas)
{
    canvas_ = &canvas;
}
void side_rulers::set_scale(unsigned axis, double scale)
{
    assert(axis < 2);
    scales_[1 - axis] = scale;
}
void side_rulers::end()
{
    if (active_)
    {
        active_ = false;
        assert(canvas_);
        if ((flags_ & RIGHT_SIDE) != 0)
            reserve_space(RIGHT_SIDE, 0);
        row_.end();
        do_ruler_row(BOTTOM_SIDE);
        grid_.end();
        draw_ruler(flags_ & flag_set(~(TOP_SIDE_CODE | BOTTOM_SIDE_CODE)), 0);
        draw_ruler(flags_ & flag_set(~(LEFT_SIDE_CODE | RIGHT_SIDE_CODE)), 1);
    }
}
void side_rulers::draw_ruler(flag_set flags, unsigned index)
{
    draw_side_ruler(*canvas_, regions_[index], ctx_->pass_state.bg_color,
        ctx_->pass_state.text_color, scales_[index], flags);
}
void side_rulers::do_ruler_row(flag_set side)
{
    if ((flags_ & side) != 0)
    {
        grid_row r(grid_);
        if ((flags_ & LEFT_SIDE) != 0)
            do_corner();
        reserve_space(side, 1);
        if ((flags_ & RIGHT_SIDE) != 0)
            do_corner();
    }
}
void side_rulers::reserve_space(flag_set side, unsigned index)
{
    float size = float(get_ruler_width(*ctx_,
        side | (flags_ & flag_set(~SIDE_MASK_CODE))));
    do_spacer(*ctx_, &regions_[index], index != 0 ?
        layout(height(size, PIXELS), GROW | NOT_PADDED) :
        layout(width(size, PIXELS), NOT_PADDED));
}
void side_rulers::do_corner()
{
    box2i region;
    do_spacer(*ctx_, &region);
    draw_filled_box(*ctx_, ctx_->pass_state.bg_color, region);
}

void zoom_to_box(canvas& canvas, box2d const& box)
{
    canvas.set_camera_position(get_center(box));
    vector2i const& canvas_size = canvas.region().size;
    if (box.size[0] != 0 && box.size[1] != 0)
    {
        canvas.set_zoom_level((std::min)(canvas_size[0] / box.size[0],
            canvas_size[1] / box.size[1]));
    }
}

void apply_zoom_box_tool(canvas& canvas, mouse_button button,
    rgba8 const& color, line_style const& style, zoom_box_tool_data* data)
{
    context& ctx = canvas.context();
    if (!data)
        data = get_data<zoom_box_tool_data>(ctx);
    if (detect_mouse_down(ctx, canvas.id(), button))
        *data = ctx.pass_state.mouse_position;
    if (ctx.event->type == RENDER_EVENT &&
        detect_drag_in_progress(ctx, canvas.id(), button))
    {
        ctx.surface->set_mouse_cursor(CROSS_CURSOR);
        point2d const& mp = ctx.pass_state.mouse_position;
        box2d box;
        for (unsigned i = 0; i < 2; ++i)
        {
            box.corner[i] = (std::min)((*data)[i], mp[i]);
            double high = (std::max)((*data)[i], mp[i]);
            box.size[i] = high - box.corner[i];
        }
        if (box.size[0] > 2 || box.size[1] > 2)
            draw_box(ctx, color, style, box);
    }
    if (detect_drag_release(ctx, canvas.id(), button))
    {
        point2d const& mp = ctx.pass_state.mouse_position;
        box2d box;
        for (unsigned i = 0; i < 2; ++i)
        {
            box.corner[i] = (std::min)((*data)[i], mp[i]);
            double high = (std::max)((*data)[i], mp[i]);
            box.size[i] = high - box.corner[i];
        }
        if (box.size[0] > 2 || box.size[1] > 2)
            zoom_to_box(canvas, box);
    }
}

void apply_zoom_wheel_tool(canvas& canvas, unsigned mods, double factor)
{
    alia::context& ctx = canvas.context();
    canvas.set_canvas_coordinates();
    if (mouse_is_inside_region(ctx, canvas.region()))
    {
        int movement = detect_wheel_movement(ctx);
        if (movement != 0)
        {
            double zoom = canvas.get_zoom_level();
            while (movement > 0)
            {
                zoom *= 1.1;
                --movement;
            }
            while (movement < 0)
            {
                zoom /= 1.1;
                ++movement;
            }
            canvas.set_zoom_level(zoom);
        }
    }
}

void apply_zoom_drag_tool(canvas& canvas, mouse_button button,
    zoom_drag_tool_data* data)
{
    alia::context& ctx = canvas.context();
    if (!data)
        data = get_data<zoom_drag_tool_data>(ctx);
    if (detect_mouse_down(ctx, canvas.id(), button))
    {
        data->start_point_on_canvas = ctx.mouse_position;
        data->start_point_in_scene = canvas_to_scene(canvas,
            point2d(ctx.mouse_position));
        data->starting_zoom = canvas.get_zoom_level();
        data->starting_camera_position = canvas.get_camera_position();
        double normal_zoom = zoom_to_fit_scene(canvas.region().size,
            canvas.scene_box().size);
        data->zoom_out_panning = clamp(
            (data->starting_zoom - normal_zoom) / normal_zoom, 0., 1.);
    }
    if (detect_drag(ctx, canvas.id(), button))
    {
        int motion = ctx.mouse_position[0] - data->start_point_on_canvas[0];
        double new_zoom = data->starting_zoom * std::pow(1.02, motion);
        canvas.set_zoom_level(new_zoom);
        {
            point2d cp =
                data->starting_camera_position + (data->start_point_in_scene -
                canvas_to_scene(canvas, point2d(data->start_point_on_canvas),
                    canvas.get_zoom_level(), data->starting_camera_position));
            if (motion < 0 && data->zoom_out_panning)
            {
                double normal_zoom = zoom_to_fit_scene(canvas.region().size,
                    canvas.scene_box().size);
                double interpolation_factor =
                    (1 / new_zoom - 1 / data->starting_zoom) /
                    (1 / normal_zoom - 1 / data->starting_zoom);
                cp +=
                    (vector2d(get_center(canvas.scene_box())) - vector2d(cp)) *
                    clamp(interpolation_factor, 0., 1.) *
                    data->zoom_out_panning;
            }
            canvas.set_camera_position(cp);
        }
    }
}

}
