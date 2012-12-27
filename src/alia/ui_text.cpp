#include <alia/ui_library.hpp>
#include <alia/ui_utilities.hpp>

#include <sstream>
#include <utility>
#include <cctype>

#include <SkTypeface.h>
#include <SkCanvas.h>
#include <SkPaint.h>
#include <SkUtils.h>

// This file implements most of the UI text functionality.
// The only exception is the text control, which is in its own file.
// The interface for this is defined in ui_library.hpp in the "TEXT" section.

namespace alia {

static void set_skia_font_info(SkPaint& paint, font const& font)
{
    paint.setTextEncoding(SkPaint::kUTF8_TextEncoding);
    paint.setTypeface(
        SkTypeface::CreateFromName(
            font.name.c_str(),
            SkTypeface::Style(
                ((font.style & BOLD) ?
                    SkTypeface::kBold : SkTypeface::kNormal) |
                ((font.style & ITALIC) ?
                    SkTypeface::kItalic : SkTypeface::kNormal))))->unref();
    paint.setTextAlign(SkPaint::kLeft_Align);
    paint.setAntiAlias(true);
    paint.setLCDRenderText(true);
    paint.setTextSize(SkFloatToScalar(font.size));
    paint.setSubpixelText(true);
    paint.setAutohinted(true);
}

static bool is_breakable_space(SkUnichar c)
{
    return
        c >= 0x09 && c <= 0x0d || c == 0x20 || c > 0x80 &&
        (c == 0x85 || c == 0x1680 || c == 0x180E ||
        c >= 0x2000 && c <= 0x200a || c == 0x2028 || c == 0x2029 ||
        c == 0x205f);
}

static bool is_line_terminator(SkUnichar c)
{
    return
        c >= 0x0a && c <= 0x0d || c > 0x80 &&
        (c == 0x85 || c == 0x2028 || c == 0x2029);
}

static utf8_ptr skip_line_terminator(utf8_string const& text)
{
    utf8_ptr p = text.begin;
    if (p < text.end)
    {
        SkUnichar c = SkUTF8_NextUnichar(&p);
        if (c == 0x0d && p != text.end)
        {
            utf8_ptr q = p;
            SkUnichar d = SkUTF8_NextUnichar(&p);
            if (d == 0x0a)
                return p;
            else
                return q;
        }
    }
    return p;
}

// Get a pointer to the first breakable space character in the given text.
// (If no such character exists, this returns a pointer to the end of the
// text.)
static utf8_ptr find_next_breakable_space(utf8_string const& text)
{
    utf8_ptr p = text.begin;
    while (p < text.end)
    {
        utf8_ptr q = p;
        if (is_breakable_space(SkUTF8_NextUnichar(&p)))
            return q;
    }
    return text.end;
}

// Get a pointer to the beginning of the next word in the given text.
// The beginning of the next word is defined as the first non-space character
// after the first space character.
// (If no such character exists, this returns a pointer to the end of the
// text.)
static utf8_ptr find_next_word_start(utf8_string const& text)
{
    utf8_ptr p = find_next_breakable_space(text);
    while (p < text.end)
    {
        utf8_ptr q = p;
        if (!is_breakable_space(SkUTF8_NextUnichar(&p)))
            return q;
    }
    return text.end;
}

// Get a pointer to the beginning of the previous word in the text.
// p is a pointer to the current position in the text.
// text is the full text.
// The beginning of the previous word is defined as the first non-space
// character before p that has a space before it.
// (If no such character exists, this returns a pointer to the start of the
// text.)
static utf8_ptr find_previous_word_start(utf8_string const& text, utf8_ptr p)
{
    // Work backwards until we find a character matching the criteria or
    // hit the beginning of the text.
    // Initializing last_character_was_space to true ensures that the first
    // iteration will not match the criteria, and thus p itself will not be
    // returned (unless it's pointing to text.begin).
    bool last_character_was_space = true;
    while (p > text.begin)
    {
	utf8_ptr q = p;
	bool is_space = is_breakable_space(SkUTF8_PrevUnichar(&p));
        if (is_space && !last_character_was_space)
	    return q;
        last_character_was_space = is_space;
    }
    return text.begin;
}

// Given a string and a position within that string, this returns the word
// that contains that position. If the position is not part of a word, then
// it returns the block of whitespace that contains it instead.
static utf8_string
get_containing_word(utf8_string const& text, utf8_ptr p)
{
    utf8_ptr q = p;
    bool is_space = is_breakable_space(SkUTF8_NextUnichar(&q));
    while (q < text.end)
    {
	utf8_ptr t = q;
	if (is_breakable_space(SkUTF8_NextUnichar(&t)) != is_space)
	    break;
	q = t;
    }
    while (p > text.begin)
    {
	utf8_ptr t = p;
	if (is_breakable_space(SkUTF8_PrevUnichar(&t)) != is_space)
	    break;
	p = t;
    }
    return utf8_string(p, q);
}

static utf8_ptr
break_text(
    SkPaint& paint, utf8_string const& text, layout_scalar width,
    bool is_full_line, bool for_editing, layout_scalar* accumulated_width,
    utf8_ptr* visible_end)
{
    utf8_ptr p = text.begin;
    layout_scalar remaining_width = width;
    while (p < text.end)
    {
        utf8_ptr next_space =
            find_next_breakable_space(utf8_string(p, text.end));
        layout_scalar word_width =
            skia_scalar_as_layout_size(paint.measureText(p, next_space - p));
        if (word_width > remaining_width)
        {
            if (is_full_line)
            {
                SkScalar measured_width;
                size_t what_will_fit =
                    paint.breakText(p, next_space - p,
                        layout_scalar_as_skia_scalar(remaining_width),
			&measured_width);
                remaining_width -= skia_scalar_as_layout_size(measured_width);
                p += what_will_fit;
            }
            break;
        }
        remaining_width -= word_width;
        p = next_space;
        utf8_ptr space_end = text.end;
        while (p < text.end)
        {
            utf8_ptr q = p;
            SkUnichar c = SkUTF8_NextUnichar(&p);
            if (is_line_terminator(c))
            {
                *visible_end = q;
                p = skip_line_terminator(utf8_string(q, text.end));
                goto line_ended;
            }
            if (!is_breakable_space(c) ||
                for_editing && q != next_space)
            {
                space_end = q;
                break;
            }
        }
        layout_scalar space_width = skia_scalar_as_layout_size(
            paint.measureText(next_space, space_end - next_space));
        p = space_end;
        remaining_width -= space_width;
        is_full_line = false;
    }
    *visible_end = p;
 line_ended:
    *accumulated_width = width - remaining_width;
    return p;
}

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

    text_display_data* data;
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

layout_requirements text_layout_node::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    data->is_wrapped = false;
    horizontal_layout_query query(
        ctx, data->layout_cacher, data->last_content_change);
    alia_if (query.update_required())
    {
        SkPaint paint;
        set_skia_font_info(paint, data->font);
        query.update(
            calculated_layout_requirements(
                skia_scalar_as_layout_size(paint.measureText(
		    data->text.c_str(), data->text.length())),
                0, 0));
    }
    alia_end
    return query.result();
}
layout_requirements text_layout_node::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_scalar assigned_width)
{
    vertical_layout_query query(
        ctx, data->layout_cacher, data->last_content_change, assigned_width);
    alia_if (query.update_required())
    {
        SkPaint paint;
        set_skia_font_info(paint, data->font);
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
    relative_region_assignment rra(
        ctx, *this, data->layout_cacher, data->last_content_change,
        assignment);
    rra.update();
}

layout_requirements text_layout_node::get_minimal_horizontal_requirements(
    layout_calculation_context& ctx)
{
    data->is_wrapped = true;
    SkPaint paint;
    set_skia_font_info(paint, data->font);
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);
    // This is kind of arbitrary, but it should rarely come into play.
    return layout_requirements(
        skia_scalar_as_layout_size(metrics.fAvgCharWidth * 10), 0, 0, 0);
}
void text_layout_node::calculate_wrapping(
    layout_calculation_context& ctx,
    layout_scalar assigned_width,
    wrapping_state& state)
{
    utf8_string text = as_utf8_string(data->text);
    if (is_empty(text))
        return;

    SkPaint paint;
    set_skia_font_info(paint, data->font);
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
        fold_in_requirements(state.active_row.requirements, y_requirements);
        layout_scalar line_width;
        utf8_ptr visible_end;
        utf8_ptr line_end =
            break_text(
                paint, utf8_string(p, text.end),
                usable_width - state.accumulated_width,
                state.accumulated_width == 0, false,
                &line_width, &visible_end);
        state.accumulated_width += line_width + padding_width;
        if (line_end == text.end)
            break;
        p = line_end;
        wrap_row(state);
    }
}
void text_layout_node::assign_wrapped_regions(
    layout_calculation_context& ctx,
    layout_scalar assigned_width,
    wrapping_assignment_state& state)
{
    data->wrapped_rows.clear();

    utf8_string text = as_utf8_string(data->text);
    if (is_empty(text))
        return;

    SkPaint paint;
    set_skia_font_info(paint, data->font);
    SkPaint::FontMetrics metrics;
    SkScalar line_spacing = paint.getFontMetrics(&metrics);

    char space = ' ';
    layout_scalar padding_width =
	skia_scalar_as_layout_size(paint.measureText(&space, 1));
    layout_scalar usable_width = assigned_width;

    data->wrapped_y = state.active_row->y;

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
        data->wrapped_rows.push_back(row);

        // Advance.
        state.x += line_width + padding_width;
        y += state.active_row->requirements.minimum_size;
        if (line_end == text.end)
            break;
        p = line_end;
        wrap_row(state);
    }

    data->wrapped_size = make_layout_vector(assigned_width, y);
}

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

void do_text(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec)
{
    text_display_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        if (!data->text_id.matches(text.id()) ||
            !data->text_valid && text.is_gettable() ||
            !data->style_id.matches(*ctx.style.id) ||
            data->layout_spec != layout_spec)
        {
            record_layout_change(get_layout_traversal(ctx));
            data->last_content_change = ctx.layout.refresh_counter;
            data->layout_node.data = data;
            data->font = ctx.style.properties->font;
            data->layout_spec = layout_spec;
            data->text_valid = text.is_gettable();
            data->text = data->text_valid ? get(text) : "";
            data->text_id.store(text.id());
            data->style_id.store(*ctx.style.id);
            update_layout_cacher(get_layout_traversal(ctx),
                data->layout_cacher, layout_spec, LEFT | BASELINE_Y);
        }
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;

     case RENDER_CATEGORY:
      {
        if (data->is_wrapped)
        {
            if (!data->wrapped_rows.empty())
            {
                layout_box region(
                    make_layout_vector(0, data->wrapped_y),
                    data->wrapped_size);
                caching_renderer cache(
                    ctx, data->rendering,
                    make_id(data->last_content_change),
                    region);
                if (cache.needs_rendering())
                {
                    skia_renderer renderer(ctx, cache.image(), region.size);
                    SkPaint paint;
                    paint.setFlags(SkPaint::kAntiAlias_Flag);
                    set_skia_font_info(paint, data->font);
                    rgba8 const& bg = ctx.style.properties->bg_color;
                    renderer.canvas().clear(
                        SkColorSetARGB(bg.a, bg.r, bg.g, bg.b));
                    set_color(paint, ctx.style.properties->text_color);
                    for (std::vector<text_display_row>::const_iterator
                        i = data->wrapped_rows.begin();
                        i != data->wrapped_rows.end(); ++i)
                    {
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
                data->layout_cacher.resolved_relative_assignment;
            if (assignment.region.size[0] > 0)
            {
                caching_renderer cache(
                    ctx, data->rendering,
                    make_id(data->last_content_change),
                    assignment.region);
                if (cache.needs_rendering())
                {
                    skia_renderer renderer(ctx, cache.image(),
                        assignment.region.size);
                    SkPaint paint;
                    paint.setFlags(SkPaint::kAntiAlias_Flag);
                    set_skia_font_info(paint, data->font);
                    rgba8 const& bg = ctx.style.properties->bg_color;
                    renderer.canvas().clear(
                        SkColorSetARGB(bg.a, bg.r, bg.g, bg.b));
                    set_color(paint, ctx.style.properties->text_color);
                    renderer.canvas().drawText(
                        data->text.c_str(), data->text.length(),
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
    flow_layout f(ctx, add_default_padding(layout_spec, PADDED));
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
                skia_scalar_as_layout_size(metrics.fAscent),
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

	rgba8 bg = ctx.style.properties->bg_color;
	renderer.canvas().clear(
	    SkColorSetARGB(bg.a, bg.r, bg.g, bg.b));

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

	rgba8 bg = ctx.style.properties->bg_color;
	renderer.canvas().clear(
	    SkColorSetARGB(bg.a, bg.r, bg.g, bg.b));

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

    if (is_rendering(ctx))
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
    layout_dependent_text_data* data;
    get_cached_data(ctx, &data);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
      {
        data->layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec,
            leaf_layout_requirements(make_layout_vector(0, 0), 0, 0));
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;
      }
     case RENDER_CATEGORY:
      {
        relative_layout_assignment const& assignment =
	    data->layout_node.assignment();
        draw_text(ctx, data->drawing, text,
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

    link_data* data;
    // Technically, the key_state field is state, but it only needs to persist
    // while the user is directly interacting with the link, so it's fine to
    // just call everything cached data.
    get_cached_data(ctx, &data);

    widget_state state = get_button_state(ctx, id, data->input);
    scoped_substyle substyle(ctx, alia::text("link"), state);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        refresh_standalone_text(ctx, data->standalone_text, text, layout_spec);
        break;

     case REGION_CATEGORY:
        do_box_region(ctx, id, get_region(data->standalone_text), HAND_CURSOR);
        break;

     case RENDER_CATEGORY:
        render_standalone_text(ctx, data->standalone_text, text);
        if (state & WIDGET_FOCUSED)
        {
            // TODO: This should be a little less hackish, but that might
            // require more information about the font metrics.
            box<2,int> const& ar = get_region(data->standalone_text);
            box<2,int> r;
            r.corner[0] = ar.corner[0] - 1;
            r.corner[1] = ar.corner[1];
            r.size[0] = ar.size[0] + 3;
            r.size[1] = ar.size[1] + 1;
            draw_focus_rect(ctx, data->focus_rect, r);
        }
        break;

     case INPUT_CATEGORY:
        return do_button_input(ctx, id, data->input);
    }

    return false;
}

// TEXT CONTROL - TODO: move to a separate file

struct text_layout_data
{
    string text; // storage for the text
    alia::font font;
    std::vector<utf8_string> rows;
    int line_height;
};

static void
calculate_text_layout(
    text_layout_data& data, string const& text, font const& font, int width,
    bool for_editing)
{
    data.text = text;
    data.font = font;

    SkPaint paint;
    set_skia_font_info(paint, font);

    SkPaint::FontMetrics metrics;
    data.line_height =
	skia_scalar_as_layout_size(paint.getFontMetrics(&metrics));

    utf8_string utf8 = as_utf8_string(data.text);

    data.rows.clear();
    char const* p = utf8.begin;
    do // Always include at least one row, even for empty strings.
    {
        layout_scalar line_width;
        utf8_ptr visible_end;
        utf8_ptr line_end =
            break_text(
                paint, utf8_string(p, utf8.end), width, true, for_editing,
                &line_width, &visible_end);
        data.rows.push_back(utf8_string(p, visible_end));
        p = line_end;
    }
    while (p != utf8.end);
}

// Get the index of the line that contains the given offset.
size_t get_line_number(text_layout_data const& layout, utf8_ptr character)
{
    size_t n_rows = layout.rows.size();
    for (size_t i = 0; i != n_rows - 1; ++i)
    {
        if (character < layout.rows[i + 1].begin)
            return i;
    }
    return n_rows - 1;
}

static layout_vector
get_character_position(text_layout_data const& layout, utf8_ptr character)
{
    size_t line_n = get_line_number(layout, character);
    utf8_ptr line_begin = layout.rows[line_n].begin;
    SkPaint paint;
    set_skia_font_info(paint, layout.font);
    return make_vector(
        skia_scalar_as_layout_size(
            paint.measureText(line_begin, character - line_begin)),
        layout_scalar(line_n * layout.line_height));
}

static utf8_ptr
get_line_begin(text_layout_data const& layout, size_t line_n)
{
    assert(line_n < layout.rows.size());
    return layout.rows[line_n].begin;
}
static utf8_ptr
get_line_end(text_layout_data const& layout, size_t line_n)
{
    assert(line_n < layout.rows.size());
    return layout.rows[line_n].end;
}

static optional<utf8_ptr>
get_character_at_point(text_layout_data const& layout, layout_vector const& p)
{
    if (p[0] < 0)
        return none;

    int row_index = int(p[1] / layout.line_height);
    if (row_index < 0 || row_index >= int(layout.rows.size()))
        none;

    utf8_string const& row_text = layout.rows[row_index];

    SkPaint paint;
    set_skia_font_info(paint, layout.font);

    size_t what_fits =
        paint.breakText(row_text.begin, row_text.end - row_text.begin,
            layout_scalar_as_skia_scalar(p[0]));

    if (what_fits == row_text.end - row_text.begin)
        return none;

    return row_text.begin + what_fits;
}

static utf8_ptr
get_character_boundary_at_point(
    text_layout_data const& layout, layout_vector const& p)
{
    int row_index = int(p[1] / layout.line_height);
    if (row_index < 0)
        return as_utf8_string(layout.text).begin;
    if (row_index >= int(layout.rows.size()))
        return as_utf8_string(layout.text).end;

    utf8_string const& row_text = layout.rows[row_index];

    SkPaint paint;
    set_skia_font_info(paint, layout.font);

    if (p[0] < 0)
        return row_text.begin;

    SkScalar measured_width;
    size_t what_fits =
        paint.breakText(row_text.begin, row_text.end - row_text.begin,
            layout_scalar_as_skia_scalar(p[0]), &measured_width);

    utf8_ptr boundary_before = row_text.begin + what_fits;

    // When text is wrapped, the end of the row's text is actually the
    // beginning of the next line, so we have to avoid return that position.
    if (!is_empty(row_text) && boundary_before == row_text.end)
    {
        SkUTF8_PrevUnichar(&boundary_before);
        return boundary_before;
    }

    utf8_ptr boundary_after = boundary_before;
    SkUTF8_NextUnichar(&boundary_after);

    // As above, avoid returning the end of the row's text.
    if (boundary_after == row_text.end)
        return boundary_before;

    SkScalar width_of_character =
        paint.measureText(boundary_before, boundary_after - boundary_before);

    // Determine if the point is on the left or right side of the character
    // and return the appropriate boundary.
    return (layout_scalar_as_skia_scalar(p[0]) - measured_width) >
	width_of_character / 2 ? boundary_after : boundary_before;
}

static void
draw_wrapped_text(
    SkCanvas& canvas, SkPaint& paint,
    std::vector<utf8_string> const& rows)
{
    SkPaint::FontMetrics metrics;
    layout_scalar line_spacing =
	skia_scalar_as_layout_size(paint.getFontMetrics(&metrics));
    layout_scalar y = skia_scalar_as_layout_scalar(
	metrics.fLeading + -metrics.fAscent);
    for (std::vector<utf8_string>::const_iterator
        i = rows.begin(); i != rows.end(); ++i)
    {
        canvas.drawText(i->begin, i->end - i->begin,
            SkIntToScalar(0), layout_scalar_as_skia_scalar(y), paint);
        y += line_spacing;
    }
}

static void
render_text_image(surface& surface, cached_image_ptr& image,
    vector<2,int> const& size, text_layout_data const& layout,
    rgba8 const& fg, rgba8 const& bg)
{
    SkPaint paint;
    set_skia_font_info(paint, layout.font);

    skia_renderer renderer(surface, image, size);

    renderer.canvas().clear(
        SkColorSetARGB(bg.a, bg.r, bg.g, bg.b));

    set_color(paint, fg);
    draw_wrapped_text(renderer.canvas(), paint, layout.rows);

    renderer.cache();
}

struct text_control_data;

struct text_control_layout_node : layout_node
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

    text_control_data* data;
};

struct text_control_data
{
    text_control_data()
      : change_detected(false)
      , change_counter(1)
      , cursor_on(false)
      , cursor_position(0)
      , editing(false)
      , first_selected(0)
      , n_selected(0)
      , true_cursor_x(-1)
      , text_edited(false)
    {}

    // flags passed in by caller (stored here to detect changes)
    ui_flag_set flags;

    // Whenever a change occurs in the control, this is set.
    bool change_detected;
    // When a change is detected, this is incremented.
    // Thus it serves as an identifier for 'versions' of the control.
    counter_type change_counter;

    // This stores information about the layout (wrapping) of the text.
    // It's keyed on change_counter and the usable width of the control.
    keyed_data<text_layout_data> text_layout;

    // the control's layout node and a cacher for that node
    text_control_layout_node layout_node;
    alia::layout_cacher layout_cacher;

    // is the cursor on?
    bool cursor_on;
    // the cursor is before the character at the given index
    size_t cursor_position;

    // in editing mode?
    bool editing;

    // the range of characters that's selected
    size_t first_selected;
    size_t n_selected;

    bool safe_to_drag;
    // When dragging, this is the character index at which the drag started.
    size_t drag_start_index;

    // When moving the cursor vertically, the horizontal position within the
    // new line is determined by the cursor's original horizontal position on
    // the line where the vertical motion started, so we have to remember that.
    int true_cursor_x;

    // the text that is currently in the text box
    string text;

    // the ID of the external value associated with the text
    owned_id external_id;

    // the ID of the text style active for this control
    owned_id style_id;

    // the font
    alia::font font;

    // true if the text in the control is different than the external value
    bool text_edited;

    // data for rendering the text
    // Both are keyed on change_counter and the usable width of the control.
    keyed_data<cached_image_ptr> unselected_image, selected_image;

    // data for rendering the focus indicator
    focus_rect_data focus_rendering;
};

string get_display_text(text_control_data& tc)
{
    if (tc.flags & PASSWORD)
        return string(tc.text.length(), '*');
    else
        return tc.text;
}

layout_requirements text_control_layout_node::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    horizontal_layout_query query(
        ctx, data->layout_cacher, data->change_counter);
    alia_if (query.update_required())
    {
        // Is there any reason to set a true minimum width?
        query.update(calculated_layout_requirements(0, 0, 0));
    }
    alia_end
    return query.result();
}
layout_requirements text_control_layout_node::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_scalar assigned_width)
{
    vertical_layout_query query(
        ctx, data->layout_cacher, data->change_counter, assigned_width);
    alia_if (query.update_required())
    {
        SkPaint paint;
        set_skia_font_info(paint, data->font);
        SkPaint::FontMetrics metrics;
        SkScalar line_spacing = paint.getFontMetrics(&metrics);

        string display_text = get_display_text(*data);
        utf8_string text = as_utf8_string(display_text);

        // Count how many lines are required to render the text at this width.
        unsigned line_count = 0;
        char const* p = text.begin;
        do // Include one line even for empty strings.
        {
            layout_scalar line_width;
            utf8_ptr visible_end;
            utf8_ptr line_end =
                break_text(
                    paint, utf8_string(p, text.end),
                    // (- 1 to leave room for the cursor)
                    assigned_width - 1, true, true,
                    &line_width, &visible_end);
            ++line_count;
            p = line_end;
        }
        while (p != text.end);

        query.update(
            calculated_layout_requirements(
                line_count * skia_scalar_as_layout_size(line_spacing),
                skia_scalar_as_layout_size(-metrics.fAscent),
                skia_scalar_as_layout_size(metrics.fDescent +
                    (line_count - 1) * line_spacing)));
    }
    alia_end
    return query.result();
}
void text_control_layout_node::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    relative_region_assignment rra(
        ctx, *this, data->layout_cacher, data->change_counter, assignment);
    rra.update();
}

static void
draw_text_with_selection(
    surface& surface,
    text_layout_data const& layout,
    cached_image_ptr const& unselected_image,
    cached_image_ptr const& selected_image,
    layout_box const& region,
    utf8_ptr selection_start,
    utf8_ptr selection_end)
{
    SkPaint paint;
    set_skia_font_info(paint, layout.font);
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);

    // q is the position on the screen that we're rendering to.
    vector<2,double> q = vector<2,double>(region.corner);
    // u is the position inside the text image that we're rendering from.
    vector<2,double> u = make_vector<double>(0, 0);

    std::vector<utf8_string>::const_iterator
        row_i = layout.rows.begin(), end_row = layout.rows.end();

    // Draw all the unselected full lines before the highlight as one big
    // subregion.
    while (row_i != end_row && row_i->end < selection_start)
        ++row_i;
    {
        layout_scalar height =
	    layout_scalar(row_i - layout.rows.begin()) * layout.line_height;
        unselected_image->draw(
            surface,
            box<2,double>(q, make_vector<double>(region.size[0], height)),
            box<2,double>(u, make_vector<double>(region.size[0], height)));
        q[1] += height;
        u[1] += height;
    }

    if (row_i == end_row)
        return;

    // Now we're on the line where the selection starts.

    // First draw all the characters before the selection.
    if (row_i->begin < selection_start)
    {
        layout_scalar width = skia_scalar_as_layout_size(
            paint.measureText(row_i->begin, selection_start - row_i->begin));
        unselected_image->draw(
            surface,
            box<2,double>(q, make_vector<double>(width, layout.line_height)),
            box<2,double>(u, make_vector<double>(width, layout.line_height)));
        q[0] += width;
        u[0] += width;
    }

    // Now, draw all the selected lines, except the last one (if it's only
    // partially selected).
    utf8_ptr char_i = selection_start;
    while (row_i->end <= selection_end)
    {
        layout_scalar width = skia_scalar_as_layout_size(
            paint.measureText(char_i, row_i->end - char_i));

        selected_image->draw(
            surface,
            box<2,double>(q, make_vector<double>(width, layout.line_height)),
            box<2,double>(u, make_vector<double>(width, layout.line_height)));

        q[0] = region.corner[0]; q[1] += layout.line_height;
        u[0] = 0; u[1] += layout.line_height;

        ++row_i;
        if (row_i == end_row)
            return;
        char_i = row_i->begin;
    }

    // Draw the last selected line.
    {
        layout_scalar width = skia_scalar_as_layout_size(
            paint.measureText(char_i, selection_end - char_i));

        selected_image->draw(
            surface,
            box<2,double>(q, make_vector<double>(width, layout.line_height)),
            box<2,double>(u, make_vector<double>(width, layout.line_height)));

        q[0] += width;
        u[0] += width;
    }

    // Draw the unselected portion of the last selected line.
    {
        layout_scalar width = skia_scalar_as_layout_size(
            paint.measureText(selection_end, row_i->end - selection_end));

        unselected_image->draw(
            surface,
            box<2,double>(q, make_vector<double>(width, layout.line_height)),
            box<2,double>(u, make_vector<double>(width, layout.line_height)));

        q[0] = region.corner[0]; q[1] += layout.line_height;
        u[0] = 0; u[1] += layout.line_height;

        ++row_i;
    }

    // Draw all the fully unselected lines after the selection as one big
    // subregion.
    if (u[1] < region.size[1])
    {
        unselected_image->draw(
            surface,
            box<2,double>(q,
                make_vector<double>(
                    region.size[0],
                    region.size[1] - u[1])),
            box<2,double>(u,
                make_vector<double>(
                    region.size[0],
                    region.size[1] - u[1])));
    }
}

struct text_control
{
 public:
    text_control(
        ui_context& ctx,
        text_control_data& data,
        accessor<string> const& value,
        layout const& layout_spec,
        ui_flag_set flags,
        widget_id id,
        optional<size_t> const& length_limit)
      : ctx(ctx), data(data), value(value),
        flags(flags), layout_spec(layout_spec), id(id),
        length_limit(length_limit)
    {}

    void do_pass()
    {
        result.event = TEXT_CONTROL_NO_EVENT;
        result.changed = false;

        cursor_id = get_widget_id(ctx);

        panel_.begin(
            ctx, text("control"),
            add_default_alignment(
                add_default_size(layout_spec, width(12, CHARS)),
                LEFT, BASELINE_Y));

        switch (ctx.event->category)
        {
         case REFRESH_CATEGORY:
            do_refresh();
            break;
         case RENDER_CATEGORY:
            update_text_layout();
            render();
            break;
         case REGION_CATEGORY:
            update_text_layout();
            do_box_region(ctx, cursor_id, get_cursor_region(), IBEAM_CURSOR);
            do_box_region(ctx, id, get_full_region(), IBEAM_CURSOR);
            break;
         case INPUT_CATEGORY:
            update_text_layout();
            do_input();
            break;
        }
    }

    text_control_result<string> result;

 private:
    bool is_password() const { return (flags & PASSWORD) != 0; }
    bool is_read_only() const { return (flags & DISABLED) != 0; }
    bool is_disabled() const { return (flags & DISABLED) != 0; }
    bool is_single_line() const { return (flags & SINGLE_LINE) != 0; }
    bool is_multiline() const { return (flags & MULTILINE) != 0; }

    box<2,int> get_full_region() const
    {
        return panel_.outer_region();
    }

    box<2,int> get_cursor_region() const
    {
        return box<2,int>(
            get_character_boundary_location(
                character_index_to_ptr(data.cursor_position)),
            make_vector<int>(1, get_text_layout().line_height));
    }

    box<2,int> const& get_text_region() const
    {
        return data.layout_cacher.resolved_relative_assignment.region;
    }

    void reset_to_external_value()
    {
        data.text = value.is_gettable() ? get(value) : "";
        data.cursor_position = data.text.length();
        on_text_change();
        data.text_edited = false;
        if (!(flags & ALWAYS_EDITING))
            exit_edit_mode();
    }

    void record_change()
    {
        data.change_detected = true;
    }

    void do_refresh()
    {
        if (!data.external_id.matches(value.id()))
        {
            // The value changed through some external program logic,
            // so update the displayed text to reflect it.
            // This also aborts any edits that may have been taking
            // place.
            reset_to_external_value();
            data.external_id.store(value.id());
        }

        if (!data.style_id.matches(*ctx.style.id))
        {
            record_change();
            data.style_id.store(*ctx.style.id);
        }

        if (flags != data.flags)
        {
            record_change();
            data.flags = flags;
        }

        update_layout_cacher(get_layout_traversal(ctx), data.layout_cacher,
            UNPADDED, BASELINE_Y | GROW_X);

        if (data.change_detected)
        {
            ++data.change_counter;
            record_layout_change(get_layout_traversal(ctx));

            data.font = ctx.style.properties->font;
            data.layout_node.data = &data;

            data.change_detected = false;
        }

        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
    }

    void update_text_layout()
    {
        refresh_keyed_data(data.text_layout,
            combine_ids(make_id(data.change_counter),
                make_id(get_text_region().size[0])));
        if (!is_valid(data.text_layout))
        {
            calculate_text_layout(data.text_layout.value,
                get_display_text(data), ctx.style.properties->font,
                // - 1 to leave room for the cursor
                get_text_region().size[0] - 1,
                // for editing
                true);
            mark_valid(data.text_layout);
        }
    }

    void render()
    {
        if (!is_visible(get_geometry_context(ctx),
                box<2,double>(get_full_region())))
        {
            return;
        }

        if (id_has_focus(ctx, id))
            draw_focus_rect(ctx, data.focus_rendering, get_full_region());

        refresh_keyed_data(data.unselected_image,
            combine_ids(make_id(data.change_counter),
                make_id(get_text_region().size[0])));
        if (!is_valid(data.unselected_image) ||
            !is_valid(data.unselected_image.value))
        {
            render_text_image(
                *ctx.surface,
                data.unselected_image.value,
                get_text_region().size,
                get_text_layout(),
                ctx.style.properties->text_color,
                ctx.style.properties->bg_color);
            mark_valid(data.unselected_image);
        }

        if (data.n_selected != 0)
        {
            refresh_keyed_data(data.selected_image,
                combine_ids(make_id(data.change_counter),
                    make_id(get_text_region().size[0])));
            if (!is_valid(data.selected_image) ||
                !is_valid(data.selected_image.value))
            {
                render_text_image(
                    *ctx.surface,
                    data.selected_image.value,
                    get_text_region().size,
                    get_text_layout(),
                    ctx.style.properties->selected_text_color,
                    ctx.style.properties->selected_bg_color);
                mark_valid(data.selected_image);
            }

            draw_text_with_selection(
                *ctx.surface,
                get_text_layout(),
                data.unselected_image.value, data.selected_image.value,
                get_text_region(),
                character_index_to_ptr(data.first_selected),
                character_index_to_ptr(data.first_selected + data.n_selected));
        }
        else
        {
            data.unselected_image.value->draw(
                *ctx.surface,
                box<2,double>(get_text_region()),
                box<2,double>(
                    make_vector(0., 0.),
                    vector<2,double>(get_text_region().size)));
        }

        if (data.cursor_on && data.editing)
        {
            vector<2,int> cursor_p =
                get_character_boundary_location(character_index_to_ptr(
                    data.cursor_position));
            bool cursor_selected =
                (data.n_selected != 0 &&
                data.cursor_position >= data.first_selected &&
                data.cursor_position <
                    data.first_selected + data.n_selected);
            ctx.surface->draw_filled_box(
                cursor_selected ?
                    ctx.style.properties->selected_text_color :
                    ctx.style.properties->text_color,
                box<2,double>(vector<2,double>(cursor_p),
                    make_vector<double>(1, get_text_layout().line_height)));
        }
    }

    text_layout_data& get_text_layout() const
    { return data.text_layout.value; }

    void do_input()
    {
        if (detect_double_click(ctx, id, LEFT_BUTTON))
        {
            string const& display_text = get_text_layout().text;
            optional<utf8_ptr> character =
                get_character_at_pixel(get_integer_mouse_position(ctx));
            if (character)
            {
		utf8_string word = get_containing_word(
		    as_utf8_string(display_text), get(character));
                set_selection(character_ptr_to_index(word.begin),
		    character_ptr_to_index(word.end));
                data.cursor_position = character_ptr_to_index(word.end);
                data.true_cursor_x = -1;
                reset_cursor_blink();
            }
        }
        else if (detect_mouse_press(ctx, id, LEFT_BUTTON))
        {
            // This determines if the click is just an initial "move the focus
            // to this control and select its text" click or a an actual click
            // to move the cursor and/or drag.
            // If the control already has focus, then all clicks are the latter
            // type. Similarly if the control is read-only. It's less clear
            // what to do for multiline controls (and what constitutes a
            // "multiline" control), so this may have to be revisited.
            if (is_read_only() || get_text_layout().rows.size() > 1 ||
                id_has_focus(ctx, id))
            {
                size_t index =
                    character_ptr_to_index(get_character_boundary_at_pixel(
                        get_integer_mouse_position(ctx)));
                data.drag_start_index = index;
                move_cursor(index);
                reset_cursor_blink();
                data.safe_to_drag = true;
                if (!is_read_only())
                    data.editing = true;
            }
            else
                data.safe_to_drag = false;
        }
        else if (detect_drag(ctx, id, LEFT_BUTTON) && data.safe_to_drag)
        {
            do_drag();
            start_timer(ctx, id, drag_delay);
        }

        if (is_timer_done(ctx, id) && is_region_active(ctx, id) &&
            is_mouse_button_pressed(ctx, LEFT_BUTTON))
        {
            do_drag();
            restart_timer(ctx, id, drag_delay);
        }

        //if (detect_click(ctx, id, RIGHT_BUTTON))
        //{
        //    right_click_menu menu(*this);
        //    ctx.surface->show_popup_menu(&menu);
        //}

        do_key_input();

        {
            if (detect_focus_gain(ctx, id))
            {
                if (!is_read_only())
                    data.editing = true;
                reset_cursor_blink();
                ensure_cursor_visible();
                if (!is_read_only() && get_line_count() < 2)
                    select_all();
            }
            else if (detect_focus_loss(ctx, id))
            {
                if (data.text_edited)
                {
                    value.set(data.text);
                    result.new_value = data.text;
                    result.event = TEXT_CONTROL_FOCUS_LOST;
                    result.changed = true;
                }
                exit_edit_mode();
            }
        }

        if (id_has_focus(ctx, id) && is_timer_done(ctx, cursor_id))
        {
            data.cursor_on = !data.cursor_on;
            restart_timer(ctx, cursor_id, cursor_blink_delay);
        }
    }

    void do_drag()
    {
        size_t index =
            character_ptr_to_index(get_character_boundary_at_pixel(
                get_integer_mouse_position(ctx)));
        set_selection(data.drag_start_index, index);
        data.cursor_position = index;
        data.true_cursor_x = -1;
        ensure_cursor_visible();
        reset_cursor_blink();
    }

    void do_key_input()
    {
        if (!is_read_only())
            add_to_focus_order(ctx, id);

        utf8_string text;
        if (detect_text_input(ctx, &text, id))
        {
            // Ignore control characters.
            // TODO: Do this in a more Unicode-aware manner.
            if (text.end != text.begin + 1 || isprint(*text.begin))
            {
                if (data.editing)
                {
                    insert_text(string(text.begin, text.end - text.begin));
                    on_edit();
                    acknowledge_key();
                }
            }
        }
        key_event_info info;
        if (detect_key_press(ctx, &info, id))
        {
            switch (info.mods.code)
            {
             case 0:
                switch (info.code)
                {
                 case KEY_HOME:
                    move_cursor(character_ptr_to_index(get_home_position()));
                    acknowledge_key();
                    break;

                 case KEY_END:
                    move_cursor(character_ptr_to_index(get_end_position()));
                    acknowledge_key();
                    break;

                 case KEY_ENTER:
                 case KEY_NUMPAD_ENTER:
                    if (data.editing)
                    {
                        if (is_multiline())
                        {
                            insert_text("\n");
                            on_edit();
                        }
                        else
                        {
                            if (data.text_edited)
                            {
                                value.set(data.text);
                                result.new_value = data.text;
                                result.changed = true;
                            }
                            if ((flags & ALWAYS_EDITING) == 0)
                                exit_edit_mode();
                            result.event = TEXT_CONTROL_ENTER_PRESSED;
                        }
                    }
                    else
                        data.editing = true;
                    acknowledge_key();
                    break;

                 case KEY_ESCAPE:
                    reset_to_external_value();
                    result.event = TEXT_CONTROL_EDIT_CANCELED;
                    acknowledge_input_event(ctx);
                    break;

                 case KEY_BACKSPACE:
                    if (data.editing)
                    {
                        if (has_selection())
                        {
                            delete_selection();
                        }
                        else if (data.cursor_position > 0)
                        {
                            data.text =
                                data.text.substr(0, data.cursor_position - 1) +
                                data.text.substr(data.cursor_position);
                            --data.cursor_position;
                        }
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_DELETE:
                    if (data.editing)
                    {
                        if (has_selection())
                        {
                            delete_selection();
                        }
                        else if (data.cursor_position <
                            int(data.text.length()))
                        {
                            data.text =
                                data.text.substr(0, data.cursor_position) +
                                data.text.substr(data.cursor_position + 1);
                        }
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_LEFT:
                    move_cursor(shifted_cursor_position(-1));
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    move_cursor(shifted_cursor_position(1));
                    acknowledge_key();
                    break;

                 case KEY_UP:
                    if (is_multiline() || get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(-1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_DOWN:
                    if (is_multiline() || get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEUP:
                    if (is_multiline() || get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(
                            -(get_text_region().size[1] /
                            get_text_layout().line_height - 1)), false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEDOWN:
                    if (is_multiline() || get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(
                            get_text_region().size[1] /
                            get_text_layout().line_height - 1), false);
                        acknowledge_key();
                    }
                    break;

                 default:
                    ;
                }
                break;

             case KMOD_CTRL_CODE:
                switch (info.code)
                {
                 case 'a':
                    select_all();
                    acknowledge_key();
                    break;

                 case 'c':
                 case KEY_INSERT:
                    copy();
                    acknowledge_key();
                    break;

                 case 'x':
                    if (data.editing)
                    {
                        cut();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case 'v':
                    if (data.editing)
                    {
                        paste();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_HOME:
                    move_cursor(0);
                    acknowledge_key();
                    break;

                 case KEY_END:
                    move_cursor(get_text_layout().text.length());
                    acknowledge_key();
                    break;

                 case KEY_DELETE:
                    if (data.editing)
                    {
                        delete_selection();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_LEFT:
                    move_cursor(character_ptr_to_index(
                        find_previous_word_start(
			    as_utf8_string(get_text_layout().text),
			    character_index_to_ptr(data.cursor_position))));
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    move_cursor(character_ptr_to_index(
                        find_next_word_start(
			    utf8_string(
			        character_index_to_ptr(data.cursor_position),
				as_utf8_string(get_text_layout().text).end))));
                    acknowledge_key();
                    break;

                 default:
                    ;
                }
                break;

             case KMOD_SHIFT_CODE:
                switch (info.code)
                {
                 case KEY_HOME:
                    shift_move_cursor(character_ptr_to_index(
                        get_home_position()));
                    acknowledge_key();
                    break;

                 case KEY_END:
                    shift_move_cursor(character_ptr_to_index(
                        get_end_position()));
                    acknowledge_key();
                    break;

                 case KEY_INSERT:
                    if (data.editing)
                    {
                        paste();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_DELETE:
                    if (data.editing)
                    {
                        cut();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case KEY_LEFT:
                    shift_move_cursor(shifted_cursor_position(-1));
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    shift_move_cursor(shifted_cursor_position(1));
                    acknowledge_key();
                    break;

                 case KEY_UP:
                    if (is_multiline() || get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(-1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_DOWN:
                    if (is_multiline() || get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEUP:
                    if (is_multiline() || get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(
                            -(get_text_region().size[1] /
                            get_text_layout().line_height - 1)), false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEDOWN:
                    if (is_multiline() || get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(
                            get_text_region().size[1] /
                            get_text_layout().line_height - 1), false);
                        acknowledge_key();
                    }
                    break;

                 default:
                    ;
                }
                break;

             case KMOD_SHIFT_CODE | KMOD_CTRL_CODE:
                switch (info.code)
                {
                 case KEY_HOME:
                    shift_move_cursor(0);
                    acknowledge_key();
                    break;

                 case KEY_END:
                    shift_move_cursor(int(data.text.length()));
                    acknowledge_key();
                    break;

                 case KEY_LEFT:
                    shift_move_cursor(character_ptr_to_index(
                        find_previous_word_start(
			    as_utf8_string(get_text_layout().text),
			    character_index_to_ptr(data.cursor_position))));
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    shift_move_cursor(character_ptr_to_index(
                        find_next_word_start(
			    utf8_string(
			        character_index_to_ptr(data.cursor_position),
				as_utf8_string(get_text_layout().text).end))));
                    acknowledge_key();
                    break;

                 default:
                    ;
                }
                break;
            }
        }
    }

    // Call this after any key press.
    void acknowledge_key()
    {
        reset_cursor_blink();
        acknowledge_input_event(ctx);
        ensure_cursor_visible();
    }

    void ensure_cursor_visible()
    {
        make_widget_visible(ctx, cursor_id);
    }

    // Reset the cursor blink so that it's visible.
    void reset_cursor_blink()
    {
        data.cursor_on = true;
        start_timer(ctx, cursor_id, cursor_blink_delay);
    }

    void on_text_change()
    {
        data.true_cursor_x = -1;
        record_change();
    }

    void on_edit()
    {
        on_text_change();
        data.text_edited = true;
    }

    void exit_edit_mode()
    {
        data.editing = false;
        data.n_selected = 0;
        data.cursor_on = false;
    }

    // Get the number of the line that contains the given character.
    size_t get_line_number(utf8_ptr character) const
    {
        return alia::get_line_number(get_text_layout(), character);
    }

    // Insert text at the current cursor position.
    void insert_text(string const& text)
    {
        if (!length_limit ||
            data.text.length() + text.length() - data.n_selected
            <= get(length_limit))
        {
            delete_selection();
            data.text = data.text.substr(0, data.cursor_position) +
                text + data.text.substr(data.cursor_position);
            data.cursor_position += text.length();
        }
    }

    // Move the cursor to the given position.
    void move_cursor(size_t new_position, bool reset_x = true)
    {
        data.cursor_position = new_position;
        data.n_selected = 0;
        if (reset_x)
            data.true_cursor_x = -1;
    }

    size_t shifted_cursor_position(int shift)
    {
        if (shift < 0)
        {
            return size_t(-shift) > data.cursor_position ?
                0 : data.cursor_position + shift;
        }
        else
        {
            return (std::min)(
                get_text_layout().text.length(),
                data.cursor_position + shift);
        }
    }

    // Move the cursor, manipulating the selection in the process.
    void shift_move_cursor(size_t new_position, bool reset_x = true)
    {
        size_t selection_end = data.first_selected + data.n_selected;

        if (has_selection() && data.cursor_position == data.first_selected)
            set_selection(new_position, selection_end);
        else if (has_selection() && data.cursor_position == selection_end)
            set_selection(data.first_selected, new_position);
        else
            set_selection(data.cursor_position, new_position);

        data.cursor_position = new_position;

        if (reset_x)
            data.true_cursor_x = -1;
    }

    // Set the current selection.
    void set_selection(size_t from, size_t to)
    {
        if (from > to)
            std::swap(from, to);
        data.first_selected = from;
        data.n_selected = to - from;
    }

    // Select all text.
    void select_all()
    {
        data.first_selected = 0;
        data.cursor_position = data.n_selected = int(data.text.length());
    }

    // Is there currently any text selected?
    bool has_selection() const
    {
        return data.n_selected != 0;
    }

    // Delete the current selection.
    void delete_selection()
    {
        if (has_selection())
        {
            data.text = data.text.substr(0, data.first_selected) +
                data.text.substr(data.first_selected + data.n_selected);
            data.cursor_position = data.first_selected;
            data.n_selected = 0;
        }
    }

    // Copy the current selection to the clipboard.
    void copy()
    {
        if (!(flags & PASSWORD) && has_selection())
        {
            ctx.surface->set_clipboard_text(
                data.text.substr(data.first_selected, data.n_selected));
        }
    }

    // Cut the current selection.
    void cut()
    {
        copy();
        delete_selection();
    }

    // Paste the current clipboard contents into the control.
    void paste()
    {
        insert_text(ctx.surface->get_clipboard_text());
    }

    // Convert back and forth between character indices and pointers.
    utf8_ptr character_index_to_ptr(size_t index) const
    {
        return get_text_layout().text.c_str() + index;
    }
    size_t character_ptr_to_index(utf8_ptr ptr) const
    {
        return ptr - get_text_layout().text.c_str();
    }

    // Get the number of the line that the cursor is on.
    size_t get_cursor_line_number() const
    {
        return get_line_number(character_index_to_ptr(
            data.cursor_position));
    }

    // Get the position that the home key should go to.
    utf8_ptr get_home_position() const
    {
        return get_line_begin(get_cursor_line_number());
    }

    // Get the position that the end key should go to.
    utf8_ptr get_end_position() const
    {
        return get_line_end(get_cursor_line_number());
    }

    // Get the number of lines of text in the current layout.
    size_t get_line_count() const
    {
        return get_text_layout().rows.size();
    }

    // Get the character index that corresponds to the cursor position shifted
    // down by delta lines (a negative delta shifts up).
    size_t get_vertically_adjusted_position(int delta)
    {
        size_t line_n = get_cursor_line_number();
        if (data.true_cursor_x < 0)
        {
            data.true_cursor_x = 
                get_character_position(get_text_layout(),
                    character_index_to_ptr(data.cursor_position))[0];
        }
        line_n = size_t(clamp(int(line_n) + delta, 0,
            int(get_line_count()) - 1));
        return character_ptr_to_index(
            get_character_boundary_at_point(
                get_text_layout(),
                make_vector<int>(
                    data.true_cursor_x,
                    get_character_position(
                        get_text_layout(),
                        get_line_begin(line_n))[1])));
    }

    // Get the index of the character that contains the given pixel.
    // Will return invalid character indices if the pixel is not actually
    // inside a character.
    optional<utf8_ptr> get_character_at_pixel(vector<2,int> const& p)
    {
        return get_character_at_point(get_text_layout(),
            vector<2,int>(p - get_text_region().corner));
    }

    utf8_ptr get_line_begin(size_t line_n) const
    {
        return alia::get_line_begin(get_text_layout(), line_n);
    }

    utf8_ptr get_line_end(size_t line_n) const
    {
        return alia::get_line_end(get_text_layout(), line_n);
    }

    // Get the index of the character that begins closest to the given pixel.
    utf8_ptr get_character_boundary_at_pixel(vector<2,int> const& p) const
    {
        return get_character_boundary_at_point(
            get_text_layout(),
            vector<2,int>(p - get_text_region().corner));
    }

    // Get the screen location of the character boundary immediately before the
    // given character index.
    vector<2,int> get_character_boundary_location(utf8_ptr character) const
    {
        return get_character_position(get_text_layout(), character) +
            vector<2,int>(get_text_region().corner);
    }

    ui_context& ctx;
    text_control_data& data;
    accessor<string> const& value;
    ui_flag_set flags;
    layout const& layout_spec;
    widget_id id, cursor_id;
    optional<size_t> length_limit;
    static int const cursor_blink_delay = 500;
    static int const drag_delay = 40;
    panel panel_;
};

text_control_result<string>
do_text_control(
    ui_context& ctx,
    accessor<string> const& value,
    layout const& layout_spec,
    ui_flag_set flags,
    widget_id id,
    optional<size_t> const& length_limit)
{
    if (!id) id = get_widget_id(ctx);
    text_control_data* data;
    get_cached_data(ctx, &data);
    text_control tc(ctx, *data, value, layout_spec, flags, id, length_limit);
    tc.do_pass();
    return tc.result;
}

}
