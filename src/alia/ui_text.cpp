#include <alia/ui_library.hpp>
#include <alia/ui_utilities.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include <alia/native/font_provider.hpp>
#include <alia/ascii_text_renderer.hpp>
#include <alia/cached_ascii_text.hpp>
#include <utility>
#include <cctype>

#include "SkTypeface.h"

// This file impleemnts all the UI text functionality.
// The interface for this is defined in ui_library.hpp in the "TEXT" section.

namespace alia {

// SKIA

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
    //paint.setLCDRenderText(true);
    paint.setTextSize(SkIntToScalar(font.size));
    paint.setXfermodeMode(SkXfermode::kSrc_Mode);
    //paint.setSubpixelText(true);
    //paint.setAutohinted(true);
}

//

typedef std::map<std::pair<alia::font,std::pair<rgba8,rgba8> >,
    alia__shared_ptr<ascii_font_image> > image_map_type;
image_map_type cached_font_images;

ascii_font_image const*
get_ascii_font_image(font const& font, rgba8 text_color, rgba8 bg_color)
{
    // TODO: GC font images that aren't in active use.
    typedef std::map<std::pair<alia::font,std::pair<rgba8,rgba8> >,
        alia__shared_ptr<ascii_font_image> > image_map_type;
    //static image_map_type cached_font_images;
    ascii_font_image* image;
    image_map_type::iterator i = cached_font_images.find(
        std::make_pair(font, std::make_pair(text_color, bg_color)));
    if (i == cached_font_images.end())
    {
        image = new ascii_font_image;
        native::create_ascii_font_image(image, font, text_color, bg_color);
        cached_font_images[
            std::make_pair(font, std::make_pair(text_color, bg_color))].
            reset(image);
    }
    else
        image = i->second.get();
    return image;
}

static ascii_font_image const*
get_font_image_for_active_style(ui_context& ctx)
{
    return get_ascii_font_image(ctx.style.properties->font,
        ctx.style.properties->text_color, ctx.style.properties->bg_color);
}

struct keyed_cached_image
{
    cached_image_ptr img;
    owned_id key;
};

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

struct text_display_data
{
    alia::font font;
    rgba8 text_color, bg_color;

    ascii_font_image const* font_image;

    owned_id text_id;
    bool text_valid;
    string text;

    layout layout_spec;
    resolved_layout_spec resolved_spec;
    text_layout_node layout_node;

    owned_id style_id;

    image<rgba8> text_image;
    text_image_state image_state;
    cached_image_ptr cached_image;
    bool image_cached;
    vector<2,int> image_position;

    int wrapped_rows;
};

layout_requirements text_layout_node::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    // If this gets called, we know that we're not wrapping, so just compute
    // the image of the text unwrapped and use that from here on out.

    if (data->image_state != UNWRAPPED_IMAGE)
    {
        ascii_text_renderer renderer(*data->font_image);
        create_image(data->text_image,
            make_vector<int>(
                renderer.measure_text(as_utf8_string(data->text)),
                data->font_image->metrics.ascent +
                data->font_image->metrics.descent));
        renderer.render_text(data->text_image.view, as_utf8_string(data->text),
            data->text_color, data->bg_color);
        data->image_cached = false;
        data->image_state = UNWRAPPED_IMAGE;
    }

    layout_requirements requirements;
    resolve_requirements(
        requirements, data->resolved_spec, 0,
        calculated_layout_requirements(data->text_image.view.size[0], 0, 0));
    return requirements;
}
layout_requirements text_layout_node::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_scalar assigned_width)
{
    layout_requirements requirements;
    resolve_requirements(
        requirements, data->resolved_spec, 1,
        calculated_layout_requirements(
            0,
            as_layout_size(data->font_image->metrics.ascent),
            as_layout_size(data->font_image->metrics.descent)));
    return requirements;
}
void text_layout_node::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    assert(data->image_state == UNWRAPPED_IMAGE);
    layout_requirements horizontal_requirements, vertical_requirements;
    resolve_requirements(
        horizontal_requirements, data->resolved_spec, 0,
        calculated_layout_requirements(data->text_image.view.size[0], 0, 0));
    resolve_requirements(
        vertical_requirements, data->resolved_spec, 1,
        calculated_layout_requirements(
            as_layout_size(data->font_image->metrics.ascent +
                data->font_image->metrics.descent),
            as_layout_size(data->font_image->metrics.ascent),
            as_layout_size(data->font_image->metrics.descent)));
    relative_layout_assignment relative_assignment;
    resolve_relative_assignment(relative_assignment, data->resolved_spec,
        assignment, horizontal_requirements, vertical_requirements);
    data->image_position = relative_assignment.region.corner;
}

layout_requirements text_layout_node::get_minimal_horizontal_requirements(
    layout_calculation_context& ctx)
{
    return layout_requirements(
        data->font_image->metrics.average_width * 10, 0, 0, 0);
}
void text_layout_node::calculate_wrapping(
    layout_calculation_context& ctx,
    layout_scalar assigned_width,
    wrapping_state& state)
{
    int padding_width = (std::max)(
        data->font_image->metrics.overhang * 2,
        data->font_image->advance[' ']);
    int usable_width = assigned_width - padding_width;
    data->wrapped_rows = 0;
    utf8_string text = as_utf8_string(data->text);
    if (!is_empty(text))
    {
        layout_requirements y_requirements(
            0,
            as_layout_size(data->font_image->metrics.ascent),
            as_layout_size(data->font_image->metrics.descent),
            0);
        ascii_text_renderer renderer(*data->font_image);
        utf8_ptr p = text.begin;
        while (1)
        {
            fold_in_requirements(state.active_row.requirements,
                y_requirements);
            utf8_ptr line_end =
                renderer.break_text(
                    utf8_string(p, text.end),
                    usable_width - state.accumulated_width,
                    state.accumulated_width == 0);
            state.accumulated_width +=
                renderer.compute_advance(utf8_string(p, line_end)) +
                padding_width;
            ++data->wrapped_rows;
            if (line_end == text.end)
                break;
            p = line_end;
            wrap_row(state);
        }
    }
}
void text_layout_node::assign_wrapped_regions(
    layout_calculation_context& ctx,
    layout_scalar assigned_width,
    wrapping_assignment_state& state)
{
    utf8_string text = as_utf8_string(data->text);

    int padding_width = (std::max)(
        data->font_image->metrics.overhang * 2,
        data->font_image->advance[' ']);
    int usable_width = assigned_width - padding_width;

    int total_height = 0;
    std::vector<wrapped_row>::const_iterator row_i = state.active_row;
    for (int i = 0; i != data->wrapped_rows; ++i)
    {
        total_height += row_i->requirements.minimum_size;
        ++row_i;
    }

    ascii_text_renderer renderer(*data->font_image);
    create_image(data->text_image,
        make_vector<int>(assigned_width, total_height));
    alia_foreach_pixel(data->text_image.view, rgba8, i, i = rgba8(0, 0, 0, 0));
    data->image_cached = false;
    data->image_state = UNWRAPPED_IMAGE;

    data->image_position = make_vector<int>(0, state.active_row->y);

    if (!is_empty(text))
    {
        ascii_text_renderer renderer(*data->font_image);
        utf8_ptr p = text.begin;
        int y = 0;
        while (1)
        {
            utf8_ptr line_end =
                renderer.break_text(
                    utf8_string(p, text.end),
                    usable_width - state.x,
                    state.x == 0);
            if (line_end != p)
            {
                renderer.render_text(
                    subimage(data->text_image.view, box<2,int>(
                        make_vector<int>(state.x,
                            y + state.active_row->requirements.minimum_ascent -
                                data->font_image->metrics.ascent),
                        make_vector<int>(assigned_width - state.x,
                            data->font_image->metrics.ascent +
                            data->font_image->metrics.descent))),
                    utf8_string(p, line_end),
                    data->text_color, data->bg_color);
                state.x += renderer.compute_advance(utf8_string(p, line_end)) +
                    padding_width;
            }
            if (line_end == text.end)
                break;
            p = line_end;
            y += state.active_row->requirements.minimum_size;
            wrap_row(state);
        }
    }
}

template<class T>
bool float_from_string(T* value, string const& str, string* message)
{
    try
    {
        *value = boost::lexical_cast<T>(str);
        return true;
    }
    catch (boost::bad_lexical_cast&)
    {
        *message = "This input expects a number.";
        return false;
    }
}

#define ALIA_FLOAT_CONVERSIONS(T) \
    bool from_string(T* value, string const& str, string* message) \
    { return float_from_string(value, str, message); } \
    string to_string(T value) \
    { return str(boost::format("%s") % value); }

ALIA_FLOAT_CONVERSIONS(float)
ALIA_FLOAT_CONVERSIONS(double)

template<class T>
bool integer_from_string(T* value, string const& str, string* message)
{
    try
    {
        long long n = boost::lexical_cast<long long>(str);
        *value = boost::numeric_cast<T>(n);
        return true;
    }
    catch (boost::bad_lexical_cast&)
    {
        *message = "This input expects an integer.";
        return false;
    }
    catch (boost::bad_numeric_cast&)
    {
        *message = "integer out of range";
        return false;
    }
}

#define ALIA_INTEGER_CONVERSIONS(T) \
    bool from_string(T* value, string const& str, string* message) \
    { return integer_from_string(value, str, message); } \
    string to_string(T value) \
    { return boost::lexical_cast<string>(value); }

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
            data->image_state = INVALID_IMAGE;
            data->layout_node.data = data;
            data->font = ctx.style.properties->font;
            data->text_color = ctx.style.properties->text_color;
            data->bg_color = ctx.style.properties->bg_color;
            data->font_image = get_ascii_font_image(
                data->font, data->text_color, data->bg_color);
            resolve_layout_spec(get_layout_traversal(ctx), data->resolved_spec,
                layout_spec, LEFT | BASELINE_Y);
            data->layout_spec = layout_spec;
            data->text_valid = text.is_gettable();
            data->text = data->text_valid ? get(text) : "";
            data->text_id.store(text.id());
            data->style_id.store(*ctx.style.id);
            data->image_cached = false;
        }
        add_layout_node(get_layout_traversal(ctx), &data->layout_node);
        break;

     case RENDER_CATEGORY:
        if (data->text_image.view.pixels)
        {
            if (!data->image_cached || !is_valid(data->cached_image))
            {
                ctx.surface->cache_image(data->cached_image,
                    make_interface(data->text_image.view));
                data->image_cached = true;
            }
            data->cached_image->draw(
                *ctx.surface,
                vector<2,double>(data->image_position));
        }
        break;
    }
}

void do_number(ui_context& ctx, char const* format,
    getter<double> const& number, layout const& layout_spec)
{
    cached_string_conversion* cache;
    get_cached_data(ctx, &cache);
    if (!cache->valid || !cache->id.matches(number.id()))
    {
        if (number.is_gettable())
        {
            cache->text = str(boost::format(format) % get(number));
            cache->valid = true;
        }
        else
            cache->valid = false;
        cache->id.store(number.id());
    }
    do_text(ctx, cached_string_conversion_accessor(cache), layout_spec);
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
        ascii_font_image const* font_image =
            get_font_image_for_active_style(ctx);
        ascii_text_renderer renderer(*font_image);

        vector<2,int> image_size = make_vector<int>(
            renderer.measure_text(as_utf8_string(get(text))),
            as_layout_size(font_image->metrics.ascent +
                font_image->metrics.descent));

        data.layout_requirements =
            leaf_layout_requirements(
                make_layout_vector(image_size[0], image_size[1]),
                as_layout_size(font_image->metrics.ascent),
                as_layout_size(font_image->metrics.descent));

        data.cached_image.reset();

        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec, data.layout_requirements,
            LEFT | BASELINE_Y);

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
        ascii_font_image const* font_image =
            get_font_image_for_active_style(ctx);
        ascii_text_renderer renderer(*font_image);
        image<rgba8> text_image;
        create_image(text_image, data.layout_requirements.size);
        renderer.render_text(
            text_image.view, as_utf8_string(get(text)),
            ctx.style.properties->text_color,
            ctx.style.properties->bg_color);
        ctx.surface->cache_image(data.cached_image,
            make_interface(text_image.view));
    }
    data.cached_image->draw(
        *ctx.surface,
        vector<2,double>(get_region(data).corner));
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

struct layout_dependent_text_data
{
    layout_leaf layout_node;
    cached_image_ptr cached_image;
};

void do_layout_dependent_text(ui_context& ctx, int n,
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
        layout_box const& region = data->layout_node.assignment().region;
        ascii_font_image const* font_image =
            get_ascii_font_image(ctx.style.properties->font,
                rgba8(0xff, 0xff, 0xff, 0xff),
                rgba8(0x00, 0x00, 0x00, 0x00));
        char text[64];
        sprintf(text, "%i", n);
        draw_ascii_text(*ctx.surface, font_image, data->cached_image,
            vector<2,double>(region.corner), text,
            ctx.style.properties->text_color);
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
    scoped_substyle substyle(ctx, const_text("link"), state);

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        refresh_standalone_text(ctx, data->standalone_text, text, layout_spec);
        //refresh_focus(ctx, id);
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
      : cursor_on(false)
      , cursor_position(0)
      , editing(false)
      , first_selected(0)
      , n_selected(0)
      , true_cursor_x(-1)
      , need_layout(false)
      , force_cursor_visible(false)
      , text_edited(false)
    {}

    // flags passed in by user
    ui_flag_set flags;

    // layout
    text_control_layout_node layout_node;
    resolved_layout_spec resolved_spec;
    layout_box assigned_region;

    // is the cursor on?
    bool cursor_on;
    // the cursor is before the character at the given index
    cached_text::offset cursor_position;

    // in editing mode?
    bool editing;

    // the range of characters that's selected
    cached_text::offset first_selected;
    unsigned n_selected;

    bool safe_to_drag;
    // when dragging, this is the character index at which the drag started
    cached_text::offset drag_start_index;

    // When moving the cursor vertically, the horizontal position within the
    // new line is determined by the cursor's original horizontal position on
    // the line where the vertical motion started, so we have to remember that.
    int true_cursor_x;

    bool need_layout;

    // Sometimes it's not possible to immediately force the cursor to be
    // visible because the text has changed and we don't know where the cursor
    // will actually be, so we need to remember to do it after the layout has
    // been recalculated.
    bool force_cursor_visible;

    // the text that is currently in the text box
    string text;

    // the ID of the external value associated with the text
    owned_id external_id;

    // the ID of the text style active for this control
    owned_id style_id;

    // true if text is different than external value
    bool text_edited;

    ascii_font_image const* font_image;
    ascii_font_image const* highlighted_font_image;

    cached_text_ptr renderer;

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
    // TODO: Set some sort of true minimum width?
    layout_requirements requirements;
    resolve_requirements(
        requirements, data->resolved_spec, 0,
        calculated_layout_requirements(0, 0, 0));
    return requirements;
}
layout_requirements text_control_layout_node::get_vertical_requirements(
    layout_calculation_context& ctx,
    layout_scalar assigned_width)
{
    // TODO: This is rather inefficient, but it's good enough for now.
    cached_text_ptr* renderer;
    get_data(ctx, &renderer);
    if (!*renderer ||
        assigned_width - 1 != (*renderer)->get_size()[0])
    {
        renderer->reset(new cached_ascii_text(
            *data->font_image, *data->highlighted_font_image,
            get_display_text(*data), assigned_width - 1));
    }

    layout_requirements requirements;
    resolve_requirements(
        requirements, data->resolved_spec, 1,
        calculated_layout_requirements(
            as_layout_size((*renderer)->get_size()[1]),
            as_layout_size(data->font_image->metrics.ascent),
            as_layout_size((*renderer)->get_size()[1] -
                as_layout_size(data->font_image->metrics.ascent))));
    return requirements;
}
void text_control_layout_node::set_relative_assignment(
    layout_calculation_context& ctx,
    relative_layout_assignment const& assignment)
{
    // TODO: Actually resolve position?
    data->assigned_region = assignment.region;
    data->assigned_region.corner[1] +=
        assignment.baseline_y - data->font_image->metrics.ascent;
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
        int max_chars)
      : ctx(ctx), data(data), value(value),
        flags(flags), layout_spec(layout_spec), id(id), max_chars(max_chars)
    {}

    void do_pass()
    {
        result.event = TEXT_CONTROL_NO_EVENT;
        result.changed = false;
        cursor_id = get_widget_id(ctx);

        panel_.begin(
            ctx, const_text("control"),
            add_default_alignment(
                add_default_size(layout_spec, width(12, CHARS)),
                LEFT, BASELINE_Y));
        switch (ctx.event->category)
        {
         case REFRESH_CATEGORY:
            do_refresh();
            break;
         case RENDER_CATEGORY:
            render();
            break;
         case REGION_CATEGORY:
            update_renderer();
            do_box_region(ctx, cursor_id, get_cursor_region(), IBEAM_CURSOR);
            do_box_region(ctx, id, get_full_region(), IBEAM_CURSOR);
            break;
         case INPUT_CATEGORY:
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

    box<2,int> get_full_region()
    {
        return panel_.outer_region();
    }

    box<2,int> get_cursor_region()
    {
        return box<2,int>(
            get_character_boundary_location(data.cursor_position),
            make_vector<int>(1, data.font_image->metrics.height));
    }

    void reset_to_external_value()
    {
        data.text = value.is_gettable() ? get(value) : "";
        data.cursor_position = cached_text::offset(data.text.length());
        on_text_change();
        data.text_edited = false;
        if ((flags & ALWAYS_EDITING) == 0)
            exit_edit_mode();
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
            data.need_layout = true;

            data.font_image = get_ascii_font_image(
                ctx.style.properties->font,
                ctx.style.properties->text_color,
                ctx.style.properties->bg_color);
            data.highlighted_font_image = get_ascii_font_image(
                ctx.style.properties->font,
                ctx.style.properties->selected_text_color,
                ctx.style.properties->selected_bg_color);

            data.style_id.store(*ctx.style.id);
        }

        if (flags != data.flags)
        {
            data.need_layout = true;
            data.flags = flags;
        }

        if (data.need_layout)
        {
            resolve_layout_spec(get_layout_traversal(ctx),
                data.resolved_spec, UNPADDED, BASELINE_Y | GROW_X);
            record_layout_change(get_layout_traversal(ctx));
            data.layout_node.data = &data;
            data.renderer.reset();
            data.need_layout = false;
        }

        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
    }

    void update_renderer()
    {
        if (!data.renderer)
        {
            data.renderer.reset(new cached_ascii_text(
                *data.font_image, *data.highlighted_font_image,
                get_display_text(data), data.assigned_region.size[0] - 1));
        }
    }

    void render()
    {
        update_renderer();

        if (id_has_focus(ctx, id))
            draw_focus_rect(ctx, data.focus_rendering, get_full_region());

        if (data.n_selected != 0)
        {
            data.renderer->draw_with_selection(
                *ctx.surface, vector<2,double>(data.assigned_region.corner),
                data.first_selected,
                data.first_selected + data.n_selected);
        }
        else
        {
            data.renderer->draw(
                *ctx.surface, vector<2,double>(data.assigned_region.corner));
        }

        if (data.cursor_on && data.editing)
        {
            vector<2,int> cursor_p = get_character_boundary_location(
                data.cursor_position);
            bool cursor_selected =
                (data.n_selected != 0 &&
                data.cursor_position >= data.first_selected &&
                data.cursor_position < cached_text::offset(
                    data.first_selected + data.n_selected));
            data.renderer->draw_cursor(
                *ctx.surface, cursor_selected, vector<2,double>(cursor_p));
        }
    }

    void do_input()
    {
        update_renderer();

        if (detect_double_click(ctx, id, LEFT_BUTTON))
        {
            string display_text = data.renderer->get_text();
            int i = get_character_at_pixel(get_integer_mouse_position(ctx));
            if (i >= 0 && i < int(display_text.length()))
            {
                int left, right;
                if (std::isspace(display_text[i]))
                {
                    left = i;
                    while (left > 0 &&
                        std::isspace(display_text[left - 1]))
                    {
                        --left;
                    }
                    right = i + 1;
                    while (right < int(display_text.length()) &&
                        std::isspace(display_text[right]))
                    {
                        ++right;
                    }
                }
                else if (std::isalnum(display_text[i]))
                {
                    left = i;
                    while (left > 0 &&
                        std::isalnum(display_text[left - 1]))
                    {
                        --left;
                    }
                    right = i + 1;
                    while (right < int(display_text.length()) &&
                        std::isalnum(display_text[right]))
                    {
                        ++right;
                    }
                }
                else
                {
                    left = i;
                    right = i + 1;
                }
                set_selection(left, right);
                data.cursor_position = right;
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
            if (is_read_only() || data.renderer->get_line_count() > 1 ||
                id_has_focus(ctx, id))
            {
                int i = get_character_boundary_at_pixel(
                    get_integer_mouse_position(ctx));
                data.drag_start_index = i;
                move_cursor(i);
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
                if (!is_read_only() && data.renderer->get_line_count() < 2)
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
        int i = get_character_boundary_at_pixel(
            get_integer_mouse_position(ctx));
        if (i < 0)
            i = 0;
        else if (i > int(data.renderer->get_text().length()))
            i = int(data.renderer->get_text().length()) - 1;
        set_selection(data.drag_start_index, i);
        data.cursor_position = i;
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
            // TODO: real unicode
            for (utf8_ptr p = text.begin; p != text.end; ++p)
            {
                if (std::isprint(*p))
                {
                    if (data.editing)
                    {
                        insert_text(string(1, *p));
                        on_edit();
                    }
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
                    move_cursor(get_home_position());
                    acknowledge_key();
                    break;

                 case KEY_END:
                    move_cursor(get_end_position());
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
                    move_cursor(data.cursor_position - 1);
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    move_cursor(data.cursor_position + 1);
                    acknowledge_key();
                    break;

                 case KEY_UP:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(-1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_DOWN:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEUP:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(
                            -(data.assigned_region.size[1] /
                            data.font_image->metrics.height - 1)), false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEDOWN:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(
                            data.assigned_region.size[1] /
                            data.font_image->metrics.height - 1), false);
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
                    move_cursor(int(data.text.length()));
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
                    move_cursor(get_previous_word_boundary(
                        data.cursor_position));
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    move_cursor(get_next_word_boundary(
                        data.cursor_position));
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
                    shift_move_cursor(get_home_position());
                    acknowledge_key();
                    break;

                 case KEY_END:
                    shift_move_cursor(get_end_position());
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
                    shift_move_cursor(data.cursor_position - 1);
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    shift_move_cursor(data.cursor_position + 1);
                    acknowledge_key();
                    break;

                 case KEY_UP:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(-1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_DOWN:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(1),
                            false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEUP:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(
                            -(data.assigned_region.size[1] /
                            data.font_image->metrics.height - 1)), false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEDOWN:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(
                            data.assigned_region.size[1] /
                            data.font_image->metrics.height - 1), false);
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
                    shift_move_cursor(get_previous_word_boundary(
                        data.cursor_position));
                    acknowledge_key();
                    break;

                 case KEY_RIGHT:
                    shift_move_cursor(get_next_word_boundary(
                        data.cursor_position));
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
        if (data.need_layout)
        {
            data.force_cursor_visible = true;
            return;
        }
        make_widget_visible(ctx, cursor_id);
        data.force_cursor_visible = false;
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
        data.need_layout = true;
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

    // Get the number of the line that contains the given character index.
    unsigned get_line_number(cached_text::offset char_i)
    {
        return data.renderer->get_line_number(char_i);
    }

    string sanitize(string const& text)
    {
        string r;
        r.reserve(text.length());
        for (unsigned i = 0; i < text.length(); ++i)
        {
            char c = text[i];
            if (std::isprint(unsigned char(c)) || c == '\n')
                r.push_back(c);
        }
        return r;
    }

    // Insert text at the current cursor position.
    void insert_text(string const& text)
    {
        string sanitized = sanitize(text);
        if (max_chars < 0 ||
            int(data.text.length() + sanitized.length() - data.n_selected)
            <= max_chars)
        {
            delete_selection();
            data.text = data.text.substr(0, data.cursor_position) +
                sanitized + data.text.substr(data.cursor_position);
            data.cursor_position += cached_text::offset(sanitized.length());
        }
    }

    // Move the cursor to the given position.
    void move_cursor(int new_position, bool reset_x = true)
    {
        data.cursor_position = clamp(new_position, 0,
            int(data.text.length()));
        data.n_selected = 0;

        if (reset_x)
            data.true_cursor_x = -1;
    }

    // Move the cursor, manipulating the selection in the process.
    void shift_move_cursor(int new_position, bool reset_x = true)
    {
        new_position = clamp(new_position, 0, int(data.text.length()));

        int selection_end = data.first_selected + data.n_selected;

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
    void set_selection(int from, int to)
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
        if (has_selection())
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

    // Get the position that the home key should go to.
    int get_home_position()
    {
        return get_line_begin(get_line_number(data.cursor_position));
    }

    // Get the position that the end key should go to.
    int get_end_position()
    {
        return get_line_end(get_line_number(data.cursor_position));
    }

    // Get the character index that corresponds to the cursor position shifted
    // down by delta lines (a negative delta shifts up).
    int get_vertically_adjusted_position(int delta)
    {
        unsigned line_n = get_line_number(data.cursor_position);
        if (data.true_cursor_x < 0)
        {
            data.true_cursor_x = data.renderer->get_character_position(
                data.cursor_position)[0];
        }
        line_n = unsigned(clamp(int(line_n) + delta, 0,
            int(data.renderer->get_line_count()) - 1));
        return data.renderer->get_character_boundary_at_point(make_vector<int>(
            data.true_cursor_x,
            data.renderer->get_character_position(get_line_begin(line_n))[1]));
    }

    // Get the index of the character that contains the given pixel.
    // Will return invalid character indices if the pixel is not actually
    // inside a character.
    int get_character_at_pixel(vector<2,int> const& p)
    {
        return data.renderer->get_character_at_point(
            vector<2,int>(p - data.assigned_region.corner));
    }

    int get_line_begin(int line_n)
    {
        return data.renderer->get_line_begin(line_n);
    }

    int get_line_end(int line_n)
    {
        return data.renderer->get_line_end(line_n);
    }

    // Get the index of the character that begins closest to the given pixel.
    int get_character_boundary_at_pixel(vector<2,int> const& p)
    {
        return data.renderer->get_character_boundary_at_point(
            vector<2,int>(p - data.assigned_region.corner));
    }

    // Get the screen location of the character boundary immediately before the
    // given character index.
    vector<2,int> get_character_boundary_location(cached_text::offset char_i)
    {
        return data.renderer->get_character_position(char_i) +
            vector<2,int>(data.assigned_region.corner);
    }

    // Get the index of the word boundary immediately before the given
    // character index.
    int get_previous_word_boundary(int char_i)
    {
        int n = char_i;
        while (n > 0 && !std::isalnum(data.renderer->get_text()[n - 1]))
            --n;
        while (n > 0 && std::isalnum(data.renderer->get_text()[n - 1]))
            --n;
        return n;
    }

    // Get the index of the word boundary immediately after the given
    // character index.
    int get_next_word_boundary(int char_i)
    {
        int n = char_i;
        while (n < int(data.renderer->get_text().length()) &&
            std::isalnum(data.renderer->get_text()[n]))
        {
            ++n;
        }
        while (n < int(data.renderer->get_text().length()) &&
            !std::isalnum(data.renderer->get_text()[n]))
        {
            ++n;
        }
        return n;
    }

    ui_context& ctx;
    text_control_data& data;
    accessor<string> const& value;
    ui_flag_set flags;
    layout const& layout_spec;
    widget_id id, cursor_id;
    int max_chars;
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
    int max_chars)
{
    if (!id) id = get_widget_id(ctx);
    text_control_data* data;
    get_data(ctx, &data);
    text_control tc(ctx, *data, value, layout_spec, flags, id, max_chars);
    tc.do_pass();
    return tc.result;
}

}
