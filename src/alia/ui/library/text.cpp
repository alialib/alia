#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

#include <sstream>
#include <utility>
#include <cctype>
#include <cstdarg>

#include <wx/utils.h>

// This file implements most of the UI library's text functionality.
// The only exception is the text control, which is in its own file.
// The interface for this is defined in ui_library.hpp in the "TEXT" section.

// NOTE/TODO: This doesn't really deal with horizontal overhand properly.
// Dealing with it properly would be complicated as it would probably require
// some interface changes between this file and the rest of alia. Currently,
// there are some hacks in place that mostly work.

namespace alia {

#ifdef _MSC_VER

int c99_snprintf(char* str, size_t size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}

int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

#endif

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
void float_from_string(T* value, string const& str)
{
    if (!string_to_value(str, value))
        throw validation_error("This input expects a number.");
}

#define ALIA_FLOAT_CONVERSIONS(T) \
    void from_string(T* value, string const& str) \
    { float_from_string(value, str); } \
    string to_string(T value) \
    { return value_to_string(value); }

ALIA_FLOAT_CONVERSIONS(float)
ALIA_FLOAT_CONVERSIONS(double)

template<class T>
void integer_from_string(T* value, string const& str)
{
    long long n;
    if (!string_to_value(str, &n))
        throw validation_error("This input expects an integer.");
    T x = T(n);
    if (x != n)
        throw validation_error("This integer is outside the supported range.");
    *value = x;
}

#define ALIA_INTEGER_CONVERSIONS(T) \
    void from_string(T* value, string const& str) \
    { integer_from_string(value, str); } \
    string to_string(T value) \
    { return value_to_string(value); }

ALIA_INTEGER_CONVERSIONS(int)
ALIA_INTEGER_CONVERSIONS(unsigned)
ALIA_INTEGER_CONVERSIONS(size_t)

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
        wrapping_state& state);
    void assign_wrapped_regions(
        layout_calculation_context& ctx,
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
                skia_scalar_as_layout_size(
                    -metrics.fAscent + metrics.fLeading),
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

layout_requirements
text_layout_node::get_minimal_horizontal_requirements(
    layout_calculation_context& ctx)
{
    text_display_data& data = *data_;

    // Record that this text is being used in a wrapping context.
    data.is_wrapped = true;

    // Don't force any particular minimum width when wrapping. The flow_layout
    // that we're wrapping inside should have its width by other factors, and
    // this text should just conform to that.
    // Also, using 1 pixel here makes it more obvious when the flow_layout was
    // relying on this value to determine its width.
    return layout_requirements(1, 0, 0, 0);
}
void text_layout_node::calculate_wrapping(
    layout_calculation_context& ctx,
    wrapping_state& state)
{
    text_display_data& data = *data_;

    utf8_string text = as_utf8_string(data.text);
    // Account for the fact that the last piece of text had extra padding for
    // overhang by removing any leading spaces.
    if (!is_empty(text) && *text.begin == ' ')
        ++text.begin;
    if (is_empty(text))
        return;

    SkPaint paint;
    set_skia_font_info(paint, data.font);
    SkPaint::FontMetrics metrics;
    SkScalar line_spacing = paint.getFontMetrics(&metrics);

    char space = ' ';
    layout_scalar padding_width =
        skia_scalar_as_layout_size(paint.measureText(&space, 1));
    layout_scalar usable_width = state.assigned_width;

    layout_requirements y_requirements(
        skia_scalar_as_layout_size(line_spacing),
        skia_scalar_as_layout_size(
            -metrics.fAscent + metrics.fLeading),
        skia_scalar_as_layout_size(metrics.fDescent),
        0);

    char const* p = text.begin;
    while (1)
    {
        layout_scalar line_width, visible_width;
        utf8_ptr visible_end;
        bool ended_on_line_terminator;
        utf8_ptr line_end =
            break_text(
                paint, utf8_string(p, text.end),
                usable_width - state.accumulated_width,
                state.accumulated_width == 0, false,
                &line_width, &visible_width, &visible_end,
                &ended_on_line_terminator);
        if (line_end == p && !state.accumulated_width)
        {
            // Avoid infinite loops.
            break;
        }
        if (line_end != p)
        {
            fold_in_requirements(state.active_row.requirements,
                y_requirements);
            state.visible_width =
                state.accumulated_width + visible_width;
            state.accumulated_width += line_width;
            p = line_end;
        }
        if (line_end == text.end)
        {
            if (ended_on_line_terminator)
            {
                state.accumulated_width += padding_width;
                wrap_row(state);
            }
            // If the last character wasn't a space, add the padding here to
            // make sure overhang is rendered.
            else if (visible_end == line_end)
                state.accumulated_width += padding_width;
            break;
        }
        state.accumulated_width += padding_width;
        wrap_row(state);
    }
}
void text_layout_node::assign_wrapped_regions(
    layout_calculation_context& ctx,
    wrapping_assignment_state& state)
{
    text_display_data& data = *data_;

    // When the row wrapping changes, things need to be re-rendered, so
    // invalidate the rendering data. (This is probably a bit conservative.)
    invalidate(data.rendering);

    data.wrapped_rows.clear();

    utf8_string text = as_utf8_string(data.text);
    // Account for the fact that the last piece of text had extra padding for
    // overhang by removing any leading spaces.
    if (!is_empty(text) && *text.begin == ' ')
        ++text.begin;
    if (is_empty(text))
        return;

    SkPaint paint;
    set_skia_font_info(paint, data.font);
    SkPaint::FontMetrics metrics;
    SkScalar line_spacing = paint.getFontMetrics(&metrics);

    char space = ' ';
    layout_scalar padding_width =
        skia_scalar_as_layout_size(paint.measureText(&space, 1));
    layout_scalar usable_width = state.assigned_width;

    data.wrapped_y = state.active_row->y;

    int y = 0;
    char const* p = text.begin;
    while (1)
    {
        // Determine line breaking.
        layout_scalar line_width, visible_width;
        utf8_ptr visible_end;
        bool ended_on_line_terminator;
        utf8_ptr line_end =
            break_text(
                paint, utf8_string(p, text.end),
                usable_width - state.x,
                state.x ==
                    calculate_initial_x(
                        state.assigned_width,
                        state.x_alignment,
                        *state.active_row),
                false,
                &line_width, &visible_width, &visible_end,
                &ended_on_line_terminator);

        // Record the row.
        text_display_row row;
        row.position = make_layout_vector(
            state.x,
            y + state.active_row->requirements.size -
                state.active_row->requirements.descent);
        row.text = utf8_string(p, visible_end);
        row.top = y;
        row.height = state.active_row->requirements.size;
        data.wrapped_rows.push_back(row);

        // Advance.
        state.x += line_width;
        y += state.active_row->requirements.size;
        if (line_end == text.end)
        {
            if (ended_on_line_terminator)
            {
                state.x += padding_width;
                wrap_row(state);
            }
            // If the last character wasn't a space, add the padding here to
            // make sure overhang is rendered.
            else if (visible_end == line_end)
                state.x += padding_width;
            break;
        }
        state.x += padding_width;
        p = line_end;
        wrap_row(state);
    }

    data.wrapped_size = make_layout_vector(state.assigned_width, y);
}

void do_text(ui_context& ctx, accessor<string> const& text,
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
                data.layout_cacher, layout_spec, LEFT | BASELINE_Y | PADDED);
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
                    // draw it here so that Skia can apply LCD text rendering.
                    rgba8 bg = ctx.style.properties->background_color;
                    bool draw_background = bg.a == 0xff;
                    if (!draw_background)
                    {
                        paint.setFlags(paint.getFlags() |
                            SkPaint::kGenA8FromLCD_Flag);
                        paint.setXfermodeMode(SkXfermode::kSrc_Mode);
                    }
                    for (std::vector<text_display_row>::const_iterator
                        i = data.wrapped_rows.begin();
                        i != data.wrapped_rows.end(); ++i)
                    {
                        if (draw_background)
                        {
                            set_color(paint, bg);
                            draw_rect(renderer.canvas(), paint,
                                layout_box_as_skia_box(layout_box(
                                    // HACK! TODO: Deal with overhang properly.
                                    make_vector(
                                        i->position[0] - 2, i->top),
                                    make_vector(
                                        region.size[0] + 4, i->height))));
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

void do_flow_text(ui_context& ctx, accessor<string> const& text,
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

static void
refresh_standalone_text(
    dataless_ui_context& ctx,
    standalone_text_data& data,
    accessor<string> const& text,
    layout const& layout_spec)
{
    if (!data.key.matches(combine_ids(ref(&text.id()), ref(ctx.style.id))))
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
                skia_scalar_as_layout_size(
                    -metrics.fAscent + metrics.fLeading),
                skia_scalar_as_layout_size(metrics.fDescent));

        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec, data.layout_requirements,
            LEFT | BASELINE_Y | PADDED);

        data.cached_image.reset();

        data.key.store(combine_ids(ref(&text.id()), ref(ctx.style.id)));
    }
    add_layout_node(get_layout_traversal(ctx), &data.layout_node);
}

static box<2,int> get_region(standalone_text_data& data)
{
    return data.layout_node.assignment().region;
}

static void
render_standalone_text(
    dataless_ui_context& ctx,
    standalone_text_data& data,
    accessor<string> const& text)
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

void do_label(ui_context& ctx, accessor<string> const& text,
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
draw_text(dataless_ui_context& ctx, text_drawing_data& data,
    accessor<string> const& text, vector<2,double> const& position,
    ui_text_drawing_flag_set flags)
{
    if (!data.key.matches(combine_ids(ref(&text.id()), ref(ctx.style.id))))
    {
        data.image.reset();
        data.key.store(combine_ids(ref(&text.id()), ref(ctx.style.id)));
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
            SkIntToScalar(0), -metrics.fAscent + metrics.fLeading,
            paint);

        data.ascent = SkScalarToDouble(-metrics.fAscent + metrics.fLeading);

        renderer.cache();
    }

    draw_full_image(*ctx.surface, data.image,
        position - make_vector<double>(0,
            (flags & ALIGN_TEXT_TOP) ? 0 : data.ascent));
}

void draw_text(ui_context& ctx, accessor<string> const& text,
    vector<2,double> const& position, ui_text_drawing_flag_set flags)
{
    text_drawing_data* data;
    get_cached_data(ctx, &data);

    if (is_render_pass(ctx))
        draw_text(ctx, *data, text, position, flags);
}

// LINK

struct link_data
{
    button_input_state input;
    standalone_text_data standalone_text;
    focus_rect_data focus_rect;
};

bool do_unsafe_link(
    ui_context& ctx,
    accessor<string> const& text,
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
        do_box_region(ctx, id, get_region(data.standalone_text),
            POINTING_HAND_CURSOR);
        break;

     case RENDER_CATEGORY:
        render_standalone_text(ctx, data.standalone_text, text);
        if (state & WIDGET_FOCUSED)
        {
            draw_focus_rect(ctx, data.focus_rect,
                add_border(get_region(data.standalone_text),
                    // TODO: Don't hardcode this.
                    as_layout_size(resolve_absolute_size(
                        get_layout_traversal(ctx),
                        size(3, 3, PIXELS)))));
        }
        break;

     case INPUT_CATEGORY:
        return do_button_input(ctx, id, data.input);
    }

    return false;
}

bool
do_unsafe_link(
    ui_context& ctx,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    layout const& layout_spec,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    auto result = do_unsafe_link(ctx, text, layout_spec, id);
    set_tooltip_message(ctx, id, tooltip);
    return result;
}

void
do_link(
    ui_context& ctx,
    accessor<string> const& text,
    action const& on_click,
    layout const& layout_spec,
    widget_id id)
{
    // It might be better to disable the link when :on_click isn't ready, but we don't
    // have disabled links at the moment.
    alia_if (on_click.is_ready())
    {
        if (do_unsafe_link(ctx, text, layout_spec, id))
        {
            perform_action(on_click);
            end_pass(ctx);
        }
    }
    alia_else
    {
        // Do a zero-size spacer just in case the caller is expecting this to take up a
        // layout slot (e.g., in a grid).
        do_spacer(ctx, size(0, 0, PIXELS));
    }
    alia_end
}

void
do_link(
    ui_context& ctx,
    accessor<string> const& text,
    accessor<string> const& tooltip,
    action const& on_click,
    layout const& layout_spec,
    widget_id id)
{
    get_widget_id_if_needed(ctx, id);
    do_link(ctx, text, on_click, layout_spec, id);
    set_tooltip_message(ctx, id, tooltip);
}

void do_url_link(
    ui_context& ctx,
    accessor<string> const& text,
    accessor<string> const& url,
    layout const& layout_spec,
    widget_id id)
{
    alia_if(is_gettable(url))
    {
        do_link(ctx, text,
            [&]()
            {
                wxLaunchDefaultBrowser(wxString(get(url)));
            },
            layout_spec,
            id);
    }
    alia_end
}

void do_styled_text(ui_context& ctx, accessor<string> const& substyle_name,
    accessor<string> const& text, layout const& layout_spec)
{
    scoped_substyle substyle(ctx, substyle_name);
    do_text(ctx, text, layout_spec);
}

void do_heading(ui_context& ctx, accessor<string> const& substyle_name,
    accessor<string> const& text, layout const& layout_spec)
{
    scoped_substyle substyle(ctx, substyle_name);
    bordered_layout margin(ctx, get_margin_property(ctx.style.path),
        layout_spec);
    do_text(ctx, text);
}

}
