#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

#include <sstream>
#include <utility>
#include <cctype>

// This file implements most of the UI library's text functionality.
// The only exception is the text control, which is in its own file.
// The interface for this is defined in ui_library.hpp in the "TEXT" section.

// NOTE/TODO: This assumes that using Skia's SkPaint::measureText establishes
// the horizontal bounds of the text, which doesn't seem like a valid
// assumption in general. However, I haven't seen a case of clipped text.
// This should be investigated further.

namespace alia {

namespace {

template<class T>
bool string_to_value(string const& str, T* value)
{
    std::istringstream s(str);
    T x;
    if (!(s >> x))
        return false;
    s >> std::ws;
    if (s.eof())
    {
        *value = x;
        return true;
    }
    return false;
}

template<class T>
string value_to_string(T const& value)
{
    std::ostringstream s;
    s << value;
    return s.str();
}

}

template<class T>
bool float_from_string(T* value, string const& str, string* message)
{
    if (!string_to_value(str, value))
    {
        *message = "This input expects a number.";
        return false;
    }
    return true;
}

#define ALIA_FLOAT_CONVERSIONS(T) \
    bool from_string(T* value, string const& str, string* message) \
    { return float_from_string(value, str, message); } \
    string to_string(T value) \
    { return value_to_string(value); }

ALIA_FLOAT_CONVERSIONS(float)
ALIA_FLOAT_CONVERSIONS(double)

template<class T>
bool integer_from_string(T* value, string const& str, string* message)
{
    long long n;
    if (!string_to_value(str, &n))
    {
        *message = "This input expects an integer.";
        return false;
    }
    T x = T(n);
    if (x != n)
    {
        *message = "integer out of range";
        return false;
    }
    *value = x;
    return true;
}

#define ALIA_INTEGER_CONVERSIONS(T) \
    bool from_string(T* value, string const& str, string* message) \
    { return integer_from_string(value, str, message); } \
    string to_string(T value) \
    { return value_to_string(value); }

ALIA_INTEGER_CONVERSIONS(int)
ALIA_INTEGER_CONVERSIONS(unsigned)

struct text_display_data;

struct text_layout_node : layout_node
{
    // implementation of normal layout interface
    layout_requirements get_horizontal_requirements(
        layout_calculation_context& ctx);
    layout_requirements get_vertical_requirements(
        layout_calculation_context& ctx,
        layout_scalar assigned_width);
    void set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

    // implementation of wrapping layout interface
    layout_requirements get_minimal_horizontal_requirements(
        layout_calculation_context& ctx);
    void calculate_wrapping(
        layout_calculation_context& ctx,
        layout_scalar assigned_width,
        wrapping_state& state);
    void assign_wrapped_regions(
        layout_calculation_context& ctx,
        layout_scalar assigned_width,
        wrapping_assignment_state& state);

    void set_data(text_display_data& data) { data_ = &data; }

 private:
    text_display_data* data_;
};

enum text_image_state
{
    INVALID_IMAGE,
    UNWRAPPED_IMAGE,
    WRAPPED_IMAGE
};

struct text_display_row
{
    utf8_string text;
    layout_vector position;
    // for drawing the background
    layout_scalar top, height;
};

struct text_display_data
{
    alia::font font;

    owned_id text_id;
    bool text_valid;
    string text;

    owned_id style_id;

    counter_type last_content_change;

    layout layout_spec;
    text_layout_node layout_node;

    bool is_wrapped;

    // If the text is not wrapped, this is used as its cacher.
    alia::layout_cacher layout_cacher;

    // If the text is wrapped, this is the information about its placement.
    std::vector<text_display_row> wrapped_rows;
    layout_vector wrapped_size;
    layout_scalar wrapped_y;

    caching_renderer_data rendering;
};

layout_requirements
text_layout_node::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    text_display_data& data = *data_;
    data.is_wrapped = false;
    horizontal_layout_query query(
        ctx, data.layout_cacher, data.last_content_change);
    alia_if (query.update_required())
    {
        SkPaint paint;
        set_skia_font_info(paint, data.font);
        query.update(
            calculated_layout_requirements(
                skia_scalar_as_layout_size(paint.measureText(
		    data.text.c_str(), data.text.length())),
                0, 0));
    }
    alia_end
    return query.result();
}
layout_requirements
text_layout_node::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_scalar assigned_width)
{
    text_display_data& data = *data_;
    vertical_layout_query query(
        ctx, data.layout_cacher, data.last_content_change, assigned_width);
    alia_if (query.update_required())
    {
        SkPaint paint;
        set_skia_font_info(paint, data.font);
        SkPaint::FontMetrics metrics;
        SkScalar line_spacing = paint.getFontMetrics(&metrics);
        query.update(
            calculated_layout_requirements(
                skia_scalar_as_layout_size(line_spacing),
                skia_scalar_as_layout_size(-metrics.fAscent),
                skia_scalar_as_layout_size(metrics.fDescent)));
    }
    alia_end
    return query.result();
}
void text_layout_node::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    text_display_data& data = *data_;
    relative_region_assignment rra(
        ctx, *this, data.layout_cacher, data.last_content_change,
        assignment);
    rra.update();
}

layout_requirements text_layout_node::get_minimal_horizontal_requirements(
    layout_calculation_context& ctx)
{
    text_display_data& data = *data_;
    data.is_wrapped = true;
    SkPaint paint;
    set_skia_font_info(paint, data.font);
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);
    // This is kind of arbitrary, but it should rarely come into play.
    //return layout_requirements(
    //    skia_scalar_as_layout_size(metrics.fAvgCharWidth * 10), 0, 0, 0);
    // metrics.fAvgCharWidth is apparently not reliable, and since this is
    // arbitrary anyway, use this instead.
    return layout_requirements(
        skia_scalar_as_layout_size(data.font.size * 6), 0, 0, 0);
}
void text_layout_node::calculate_wrapping(
    layout_calculation_context& ctx,
    layout_scalar assigned_width,
    wrapping_state& state)
{
    text_display_data& data = *data_;

    utf8_string text = as_utf8_string(data.text);
    if (is_empty(text))
        return;

    SkPaint paint;
    set_skia_font_info(paint, data.font);
    SkPaint::FontMetrics metrics;
    SkScalar line_spacing = paint.getFontMetrics(&metrics);

    char space = ' ';
    layout_scalar padding_width =
	skia_scalar_as_layout_size(paint.measureText(&space, 1));
    layout_scalar usable_width = assigned_width;

    layout_requirements y_requirements(
        skia_scalar_as_layout_size(line_spacing),
        skia_scalar_as_layout_size(-metrics.fAscent),
        skia_scalar_as_layout_size(metrics.fDescent),
        0);
    char const* p = text.begin;
    while (1)
    {
        layout_scalar line_width;
        utf8_ptr visible_end;
        utf8_ptr line_end =
            break_text(
                paint, utf8_string(p, text.end),
                usable_width - state.accumulated_width,
                state.accumulated_width == 0, false,
                &line_width, &visible_end);
        if (line_end != p)
        {
            fold_in_requirements(state.active_row.requirements,
                y_requirements);
            state.accumulated_width += line_width + padding_width;
            p = line_end;
        }
        if (line_end == text.end)
            break;
        wrap_row(state);
    }
}
void text_layout_node::assign_wrapped_regions(
    layout_calculation_context& ctx,
    layout_scalar assigned_width,
    wrapping_assignment_state& state)
{
    text_display_data& data = *data_;

    data.wrapped_rows.clear();

    utf8_string text = as_utf8_string(data.text);
    if (is_empty(text))
        return;

    SkPaint paint;
    set_skia_font_info(paint, data.font);
    SkPaint::FontMetrics metrics;
    SkScalar line_spacing = paint.getFontMetrics(&metrics);

    char space = ' ';
    layout_scalar padding_width =
	skia_scalar_as_layout_size(paint.measureText(&space, 1));
    layout_scalar usable_width = assigned_width;

    data.wrapped_y = state.active_row->y;

    int y = 0;
    char const* p = text.begin;
    while (1)
    {
        // Determine line breaking.
        layout_scalar line_width;
        utf8_ptr visible_end;
        utf8_ptr line_end =
            break_text(
                paint, utf8_string(p, text.end),
                usable_width - state.x,
                state.x == 0, false,
                &line_width, &visible_end);

        // Record the row.
        text_display_row row;
        row.position = make_layout_vector(
            state.x,
            y + state.active_row->requirements.minimum_size -
                state.active_row->requirements.minimum_descent);
        row.text = utf8_string(p, visible_end);
        row.top = y;
        row.height = state.active_row->requirements.minimum_size;
        data.wrapped_rows.push_back(row);

        // Advance.
        state.x += line_width + padding_width;
        y += state.active_row->requirements.minimum_size;
        if (line_end == text.end)
            break;
        p = line_end;
        wrap_row(state);
    }

    data.wrapped_size = make_layout_vector(assigned_width, y);
}

void do_text(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(text_display_data)

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        if (!data.text_id.matches(text.id()) ||
            !data.text_valid && text.is_gettable() ||
            !data.style_id.matches(*ctx.style.id) ||
            data.layout_spec != layout_spec)
        {
            record_layout_change(get_layout_traversal(ctx));
            data.last_content_change = get_refresh_counter(ctx);
            data.layout_node.set_data(data);
            data.font = ctx.style.properties->font;
            data.layout_spec = layout_spec;
            data.text_valid = text.is_gettable();
            data.text = data.text_valid ? get(text) : "";
            data.text_id.store(text.id());
            data.style_id.store(*ctx.style.id);
            update_layout_cacher(get_layout_traversal(ctx),
                data.layout_cacher, layout_spec, LEFT | BASELINE_Y);
        }
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;

     case RENDER_CATEGORY:
      {
        if (data.is_wrapped)
        {
            if (!data.wrapped_rows.empty())
            {
                layout_box region(
                    make_layout_vector(0, data.wrapped_y),
                    data.wrapped_size);
                caching_renderer cache(
                    ctx, data.rendering,
                    make_id(data.last_content_change),
                    region);
                if (cache.needs_rendering())
                {
                    skia_renderer renderer(ctx, cache.image(), region.size);
                    SkPaint paint;
                    paint.setFlags(SkPaint::kAntiAlias_Flag);
                    set_skia_font_info(paint, data.font);
                    // If the background is completely opaque, then we should
                    // draw it here so that Skia can apply proper LCD text
                    // rendering.
                    bool draw_background =
                        ctx.style.properties->background_color.a == 0xff;
                    if (!draw_background)
                    {
                        paint.setFlags(paint.getFlags() |
                            SkPaint::kGenA8FromLCD_Flag);
                        paint.setXfermodeMode(SkXfermode::kSrc_Mode);
                    }
                    rgba8 bg = ctx.style.properties->background_color;
                    for (std::vector<text_display_row>::const_iterator
                        i = data.wrapped_rows.begin();
                        i != data.wrapped_rows.end(); ++i)
                    {
                        if (draw_background)
                        {
                            set_color(paint,
                                ctx.style.properties->background_color);
                            draw_rect(renderer.canvas(), paint,
                                layout_box(
                                    make_vector(i->position[0], i->top),
                                    make_vector(region.size[0], i->height)));
                        }
                        set_color(paint, ctx.style.properties->text_color);
                        renderer.canvas().drawText(
                            i->text.begin, i->text.end - i->text.begin,
                            layout_scalar_as_skia_scalar(i->position[0]),
                            layout_scalar_as_skia_scalar(i->position[1]),
			    paint);
                    }
                    renderer.cache();
                    cache.mark_valid();
                }
                cache.draw();
            }
        }
        else
        {
            relative_layout_assignment const& assignment =
                get_assignment(data.layout_cacher);
            if (assignment.region.size[0] > 0)
            {
                caching_renderer cache(
                    ctx, data.rendering,
                    make_id(data.last_content_change),
                    assignment.region);
                if (cache.needs_rendering())
                {
                    skia_renderer renderer(ctx, cache.image(),
                        assignment.region.size);
                    SkPaint paint;
                    paint.setFlags(SkPaint::kAntiAlias_Flag);
                    set_skia_font_info(paint, data.font);
                    // If the background is completely opaque, then we should
                    // draw it here so that Skia can apply LCD text rendering.
                    rgba8 bg = ctx.style.properties->background_color;
                    if (bg.a == 0xff)
                    {
                        renderer.canvas().clear(
                            SkColorSetARGB(bg.a, bg.r, bg.g, bg.b));
                    }
                    else
                    {
                        paint.setXfermodeMode(SkXfermode::kSrc_Mode);
                        paint.setFlags(paint.getFlags() |
                            SkPaint::kGenA8FromLCD_Flag);
                    }
                    set_color(paint, ctx.style.properties->text_color);
                    renderer.canvas().drawText(
                        data.text.c_str(), data.text.length(),
                        0,
			layout_scalar_as_skia_scalar(assignment.baseline_y),
			paint);
                    renderer.cache();
                    cache.mark_valid();
                }
                cache.draw();
            }
        }
        break;
      }
    }
}

cached_string_conversion_accessor
format_number(ui_context& ctx, char const* format,
    getter<double> const& number)
{
    cached_string_conversion* cache;
    get_cached_data(ctx, &cache);
    if (!cache->valid || !cache->id.matches(number.id()))
    {
        if (number.is_gettable())
        {
            char buffer[64];
            sprintf(buffer, format, get(number));
            cache->text = buffer;
            cache->valid = true;
        }
        else
            cache->valid = false;
        cache->id.store(number.id());
    }
    return cached_string_conversion_accessor(cache);
}

void do_paragraph(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec)
{
    flow_layout f(ctx, add_default_y_alignment(add_default_padding(
        layout_spec, PADDED), BASELINE_Y));
    do_text(ctx, text);
}

struct standalone_text_data
{
    owned_id key;

    layout_leaf layout_node;
    leaf_layout_requirements layout_requirements;

    cached_image_ptr cached_image;
};

static void refresh_standalone_text(
    ui_context& ctx,
    standalone_text_data& data,
    getter<string> const& text,
    layout const& layout_spec)
{
    if (!data.key.matches(combine_ids(ref(text.id()), ref(*ctx.style.id))))
    {
        SkPaint paint;
        set_skia_font_info(paint, ctx.style.properties->font);
        SkPaint::FontMetrics metrics;
        SkScalar line_spacing = paint.getFontMetrics(&metrics);

	string const& text_value = get(text);

	SkScalar text_width =
	    paint.measureText(text_value.c_str(), text_value.length());

        data.layout_requirements =
            leaf_layout_requirements(
                make_vector(
                    skia_scalar_as_layout_size(text_width),
                    skia_scalar_as_layout_size(line_spacing)),
                skia_scalar_as_layout_size(-metrics.fAscent),
                skia_scalar_as_layout_size(metrics.fDescent));

        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec, data.layout_requirements,
            LEFT | BASELINE_Y);

	data.cached_image.reset();

        data.key.store(combine_ids(ref(text.id()), ref(*ctx.style.id)));
    }
    add_layout_node(get_layout_traversal(ctx), &data.layout_node);
}

static box<2,int> get_region(standalone_text_data& data)
{
    return data.layout_node.assignment().region;
}

static void render_standalone_text(
    ui_context& ctx,
    standalone_text_data& data,
    getter<string> const& text)
{
    if (!is_valid(data.cached_image))
    {
	SkPaint paint;
	set_skia_font_info(paint, ctx.style.properties->font);

	SkPaint::FontMetrics metrics;
	paint.getFontMetrics(&metrics);

	skia_renderer renderer(*ctx.surface, data.cached_image,
	    get_region(data).size);

	rgba8 bg = ctx.style.properties->background_color;
        // If the background is completely opaque, then we should
        // draw it here so that Skia can apply LCD text rendering.
        if (bg.a == 0xff)
        {
            renderer.canvas().clear(
                SkColorSetARGB(bg.a, bg.r, bg.g, bg.b));
        }
        else
        {
            paint.setXfermodeMode(SkXfermode::kSrc_Mode);
            paint.setFlags(paint.getFlags() |
                SkPaint::kGenA8FromLCD_Flag);
        }

	string const& text_value = get(text);

	set_color(paint, ctx.style.properties->text_color);
	renderer.canvas().drawText(
	    text_value.c_str(), text_value.length(),
	    SkIntToScalar(0),
	    layout_scalar_as_skia_scalar(
		data.layout_node.assignment().baseline_y),
	    paint);

	renderer.cache();
    }
    data.cached_image->draw(
        *ctx.surface,
        box<2,double>(get_region(data)),
        box<2,double>(
            make_vector(0., 0.),
            vector<2,double>(get_region(data).size)));
}

void do_label(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec)
{
    standalone_text_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        refresh_standalone_text(ctx, *data, text, layout_spec);
        break;
     case RENDER_CATEGORY:
        render_standalone_text(ctx, *data, text);
        break;
    }
}

struct text_drawing_data
{
    owned_id key;
    cached_image_ptr image;
    double ascent;
};

static void
draw_text(ui_context& ctx, text_drawing_data& data,
    getter<string> const& text, vector<2,double> const& position)
{
    if (!data.key.matches(combine_ids(ref(text.id()), ref(*ctx.style.id))))
    {
	data.image.reset();
	data.key.store(combine_ids(ref(text.id()), ref(*ctx.style.id)));
    }

    if (!is_valid(data.image))
    {
	SkPaint paint;
	set_skia_font_info(paint, ctx.style.properties->font);

	SkPaint::FontMetrics metrics;
	SkScalar line_spacing = paint.getFontMetrics(&metrics);

	string const& text_value = get(text);

	SkScalar text_width =
	    paint.measureText(text_value.c_str(), text_value.length());

        vector<2,int> image_size = make_vector<int>(
	    SkScalarCeilToInt(text_width),
	    SkScalarCeilToInt(line_spacing));

	skia_renderer renderer(*ctx.surface, data.image, image_size);

	rgba8 bg = ctx.style.properties->background_color;
        // If the background is completely opaque, then we should
        // draw it here so that Skia can apply LCD text rendering.
        if (bg.a == 0xff)
        {
            renderer.canvas().clear(
                SkColorSetARGB(bg.a, bg.r, bg.g, bg.b));
        }
        else
        {
            paint.setXfermodeMode(SkXfermode::kSrc_Mode);
            paint.setFlags(paint.getFlags() |
                SkPaint::kGenA8FromLCD_Flag);
        }

	set_color(paint, ctx.style.properties->text_color);
	renderer.canvas().drawText(
	    text_value.c_str(), text_value.length(),
	    SkIntToScalar(0), -metrics.fAscent,
	    paint);

	data.ascent = SkScalarToDouble(-metrics.fAscent);

	renderer.cache();
    }

    draw_full_image(*ctx.surface, data.image,
        position - make_vector<double>(0, data.ascent));
}

void draw_text(ui_context& ctx, getter<string> const& text,
    vector<2,double> const& position)
{
    text_drawing_data* data;
    get_cached_data(ctx, &data);

    if (is_render_pass(ctx))
	draw_text(ctx, *data, text, position);
}

struct layout_dependent_text_data
{
    layout_leaf layout_node;
    text_drawing_data drawing;
};

void do_layout_dependent_text(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec)
{
    ALIA_GET_CACHED_DATA(layout_dependent_text_data)

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }
     case RENDER_CATEGORY:
      {
        relative_layout_assignment const& assignment =
	    data.layout_node.assignment();
        draw_text(ctx, data.drawing, text,
            vector<2,double>(assignment.region.corner +
	        make_layout_vector(0, assignment.baseline_y)));
        break;
      }
    }
}

// LINK

struct link_data
{
    button_input_state input;
    standalone_text_data standalone_text;
    focus_rect_data focus_rect;
};

bool do_link(
    ui_context& ctx,
    getter<string> const& text,
    layout const& layout_spec,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);

    // Technically, the key_state field is state, but it only needs to persist
    // while the user is directly interacting with the link, so it's fine to
    // just call everything cached data.
    ALIA_GET_CACHED_DATA(link_data)

    widget_state state = get_button_state(ctx, id, data.input);
    scoped_substyle substyle(ctx, alia::text("link"), state);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        refresh_standalone_text(ctx, data.standalone_text, text, layout_spec);
        break;

     case REGION_CATEGORY:
        do_box_region(ctx, id, get_region(data.standalone_text), HAND_CURSOR);
        break;

     case RENDER_CATEGORY:
        render_standalone_text(ctx, data.standalone_text, text);
        if (state & WIDGET_FOCUSED)
        {
            // TODO: This should be a little less hackish, but that might
            // require more information about the font metrics.
            box<2,int> const& ar = get_region(data.standalone_text);
            box<2,int> r;
            r.corner[0] = ar.corner[0] - 1;
            r.corner[1] = ar.corner[1];
            r.size[0] = ar.size[0] + 3;
            r.size[1] = ar.size[1] + 1;
            draw_focus_rect(ctx, data.focus_rect, r);
        }
        break;

     case INPUT_CATEGORY:
        return do_button_input(ctx, id, data.input);
    }

    return false;
}

}
