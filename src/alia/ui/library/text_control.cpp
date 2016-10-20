#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

#include <utf8.h>

// This file implements alia's text control.

// NOTE/TODO: This assumes that using Skia's SkPaint::measureText establishes
// the horizontal bounds of the text, which doesn't seem like a valid
// assumption in general. However, I haven't seen a case of clipped text.
// This should be investigated further.

namespace alia {

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

    bool ended_on_line_terminator = false;

    data.rows.clear();
    char const* p = utf8.begin;
    do // Always include at least one row, even for empty strings.
    {
        layout_scalar line_width, visible_width;
        utf8_ptr visible_end;
        utf8_ptr line_end =
            break_text(
                paint, utf8_string(p, utf8.end), width, true, for_editing,
                &line_width, &visible_width, &visible_end,
                &ended_on_line_terminator);
        data.rows.push_back(utf8_string(p, visible_end));
        if (line_end == p)
        {
            // Nothing is fitting, so we're in an infinite loop. Just abort.
            break;
        }
        p = line_end;
    }
    while (p != utf8.end);

    if (ended_on_line_terminator)
        data.rows.push_back(utf8_string(utf8.end, utf8.end));
}

// Ambiguities occur when attemping to map a character offset to a cursor
// position. This is because the end of a word wrapped line is at the same
// character offset as the beginning of the next line. Both are valid cursor
// positions under different circumstances.
// This structure addresses this problem by recording not only the character
// offset by also how to resolve such an ambiguity;
struct disambiguated_utf8_ptr
{
    utf8_ptr ptr;
    bool prefer_end_of_line;

    disambiguated_utf8_ptr() {}
    explicit disambiguated_utf8_ptr(
        utf8_ptr ptr, bool prefer_end_of_line = false)
      : ptr(ptr), prefer_end_of_line(prefer_end_of_line)
    {}
};

// analogous to disambiguated_utf8_ptr, but stores an offset instead
struct disambiguated_utf8_offset
{
    size_t offset;
    bool prefer_end_of_line;

    disambiguated_utf8_offset() {}
    explicit disambiguated_utf8_offset(
        size_t offset, bool prefer_end_of_line = false)
      : offset(offset), prefer_end_of_line(prefer_end_of_line)
    {}
};

// Get the index of the line that contains the given character.
static size_t
get_line_number(
    text_layout_data const& layout, disambiguated_utf8_ptr const& character)
{
    size_t n_rows = layout.rows.size();
    for (size_t i = 0; i != n_rows - 1; ++i)
    {
        if (character.ptr <= layout.rows[i].end &&
            (character.ptr < layout.rows[i + 1].begin ||
             character.prefer_end_of_line))
        {
            return i;
        }
    }
    return n_rows - 1;
}

static layout_vector
get_character_position(text_layout_data const& layout,
    disambiguated_utf8_ptr const& character)
{
    size_t line_n = get_line_number(layout, character);
    utf8_ptr line_begin = layout.rows[line_n].begin;
    SkPaint paint;
    set_skia_font_info(paint, layout.font);
    return make_vector(
        skia_scalar_as_layout_size(
            paint.measureText(line_begin, character.ptr - line_begin)),
        layout_scalar(line_n * layout.line_height));
}

static disambiguated_utf8_ptr
get_line_begin(text_layout_data const& layout, size_t line_n)
{
    assert(line_n < layout.rows.size());
    return disambiguated_utf8_ptr(layout.rows[line_n].begin, false);
}
static disambiguated_utf8_ptr
get_line_end(text_layout_data const& layout, size_t line_n)
{
    assert(line_n < layout.rows.size());
    return disambiguated_utf8_ptr(layout.rows[line_n].end, true);
}

static optional<utf8_ptr>
get_character_at_point(text_layout_data const& layout, layout_vector const& p)
{
    if (p[0] < 0)
        return none;

    int row_index = int(p[1] / layout.line_height);
    if (row_index < 0 || row_index >= int(layout.rows.size()))
        return none;

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

static disambiguated_utf8_ptr
get_character_boundary_at_point(
    text_layout_data const& layout, layout_vector const& p)
{
    int row_index = int(p[1] / layout.line_height);
    if (row_index < 0)
        return disambiguated_utf8_ptr(as_utf8_string(layout.text).begin);
    if (row_index >= int(layout.rows.size()))
        return disambiguated_utf8_ptr(as_utf8_string(layout.text).end);

    utf8_string const& row_text = layout.rows[row_index];

    SkPaint paint;
    set_skia_font_info(paint, layout.font);

    if (p[0] < 0)
        return disambiguated_utf8_ptr(row_text.begin);

    SkScalar measured_width;
    size_t what_fits =
        paint.breakText(row_text.begin, row_text.end - row_text.begin,
            layout_scalar_as_skia_scalar(p[0]), &measured_width);

    utf8_ptr boundary_before = row_text.begin + what_fits;

    if (boundary_before == row_text.end)
        return disambiguated_utf8_ptr(row_text.end, true);

    utf8_ptr boundary_after = boundary_before;
    utf8::next(boundary_after, row_text.end);

    SkScalar width_of_character =
        paint.measureText(boundary_before,
            boundary_after - boundary_before);

    // Determine if the point is on the left or right side of the character
    // and return the appropriate boundary.
    return
        (layout_scalar_as_skia_scalar(p[0]) - measured_width) >
            width_of_character / 2
      ? disambiguated_utf8_ptr(boundary_after, boundary_after == row_text.end)
      : disambiguated_utf8_ptr(boundary_before);
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

    void set_data(text_control_data& data) { data_ = &data; }

 private:
    text_control_data* data_;
};

struct text_control_data
{
    // flags passed in by caller (stored here to detect changes)
    text_control_flag_set flags;

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
    // the cursor is before the character at the given offset
    disambiguated_utf8_offset cursor_position;

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

    // history of values for undo/redo
    // TODO: more compact representation of this?
    std::vector<string> history_buffer;
    // When undoing and redoing, this gives the index of the currently
    // selected value within the history buffer.
    size_t undo_index;

    text_control_data()
      : change_detected(false)
      , change_counter(1)
      , cursor_on(false)
      , cursor_position(disambiguated_utf8_offset(0))
      , editing(false)
      , first_selected(0)
      , n_selected(0)
      , true_cursor_x(-1)
      , text_edited(false)
      , undo_index(0)
    {}
};

static string
get_display_text(text_control_data& tc)
{
    if (tc.flags & TEXT_CONTROL_MASK_CONTENTS)
        return string(tc.text.length(), '*');
    else
        return tc.text;
}

layout_requirements
text_control_layout_node::get_horizontal_requirements(
    layout_calculation_context& ctx)
{
    text_control_data& data = *data_;
    horizontal_layout_query query(
        ctx, data.layout_cacher, data.change_counter);
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
    text_control_data& data = *data_;
    vertical_layout_query query(
        ctx, data.layout_cacher, data.change_counter, assigned_width);
    alia_if (query.update_required())
    {
        SkPaint paint;
        set_skia_font_info(paint, data.font);
        SkPaint::FontMetrics metrics;
        SkScalar line_spacing = paint.getFontMetrics(&metrics);

        string display_text = get_display_text(data);
        utf8_string text = as_utf8_string(display_text);

        // Count how many lines are required to render the text at this width.
        unsigned line_count = 0;
        char const* p = text.begin;
        bool ended_on_line_terminator = false;
        do // Include one line even for empty strings.
        {
            layout_scalar line_width, visible_width;
            utf8_ptr visible_end;
            utf8_ptr line_end =
                break_text(
                    paint, utf8_string(p, text.end),
                    // (- 1 to leave room for the cursor)
                    assigned_width - 1, true, true,
                    &line_width, &visible_width, &visible_end,
                    &ended_on_line_terminator);
            ++line_count;
            if (line_end == p)
            {
                // Nothing is fitting, so we're in an infinite loop.
                // Just abort.
                break;
            }
            p = line_end;
        }
        while (p != text.end);
        if (ended_on_line_terminator)
            ++line_count;

        query.update(
            calculated_layout_requirements(
                line_count * skia_scalar_as_layout_size(line_spacing),
                skia_scalar_as_layout_size(
                    -metrics.fAscent + metrics.fLeading),
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
    text_control_data& data = *data_;
    relative_region_assignment rra(
        ctx, *this, data.layout_cacher, data.change_counter, assignment);
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

static int const cursor_blink_delay = 500;
static int const drag_delay = 40;

struct text_control_parameters
{
    ui_context* ctx;
    text_control_data* data;
    accessor<string> const* value;
    text_control_flag_set flags;
    layout const* layout_spec;
    widget_id id;
    optional<size_t> length_limit;
    panel* panel; // containing panel
    text_control_result* result;
    validation_error_handler_data* validation;
};

text_layout_data& get_text_layout(text_control_parameters const& tc)
{ return tc.data->text_layout.value; }

static box<2,int>
get_full_region(text_control_parameters const& tc)
{
    return tc.panel->outer_region();
}

static box<2,int> const&
get_text_region(text_control_parameters const& tc)
{
    return get_assignment(tc.data->layout_cacher).region;
}

// Get the index of the character that contains the given pixel.
// Will return invalid character indices if the pixel is not actually
// inside a character.
static optional<utf8_ptr>
get_character_at_pixel(
    text_control_parameters const& tc, vector<2,int> const& p)
{
    return get_character_at_point(get_text_layout(tc),
        vector<2,int>(p - get_text_region(tc).corner));
}

static disambiguated_utf8_ptr
get_line_begin(text_control_parameters const& tc, size_t line_n)
{
    return get_line_begin(get_text_layout(tc), line_n);
}

static disambiguated_utf8_ptr
get_line_end(text_control_parameters const& tc, size_t line_n)
{
    return get_line_end(get_text_layout(tc), line_n);
}

// Get the index of the character that begins closest to the given pixel.
static disambiguated_utf8_ptr
get_character_boundary_at_pixel(
    text_control_parameters const& tc, vector<2,int> const& p)
{
    return get_character_boundary_at_point(
        get_text_layout(tc),
        vector<2,int>(p - get_text_region(tc).corner));
}

// Get the screen location of the character boundary immediately before
// the given character index.
static vector<2,int>
get_character_boundary_location(
    text_control_parameters const& tc, disambiguated_utf8_ptr const& character)
{
    return get_character_position(get_text_layout(tc), character) +
        vector<2,int>(get_text_region(tc).corner);
}

// Convert back and forth between character indices and pointers.
static utf8_ptr
character_index_to_ptr(text_control_parameters const& tc, size_t index)
{
    return get_text_layout(tc).text.c_str() + index;
}
static size_t
character_ptr_to_index(text_control_parameters const& tc, utf8_ptr ptr)
{
    return ptr - get_text_layout(tc).text.c_str();
}

// Convert back and forth between disambiguated character indices and pointers.
static disambiguated_utf8_ptr
character_index_to_ptr(
    text_control_parameters const& tc, disambiguated_utf8_offset index)
{
    return disambiguated_utf8_ptr(
        character_index_to_ptr(tc, index.offset),
        index.prefer_end_of_line);
}
static disambiguated_utf8_offset
character_ptr_to_index(
    text_control_parameters const& tc, disambiguated_utf8_ptr ptr)
{
    return disambiguated_utf8_offset(
        character_ptr_to_index(tc, ptr.ptr),
        ptr.prefer_end_of_line);
}

// Get the number of the line that contains the given character.
static size_t
get_line_number(
    text_control_parameters const& tc, disambiguated_utf8_ptr character)
{
    return alia::get_line_number(get_text_layout(tc), character);
}

// Get the number of lines of text in the current layout.
static size_t
get_line_count(text_control_parameters const& tc)
{
    return get_text_layout(tc).rows.size();
}

static widget_id
get_cursor_id(text_control_parameters const& tc)
{
    return &tc.data->cursor_position;
}

static box<2,int>
get_cursor_region(text_control_parameters const& tc)
{
    return box<2,int>(
        get_character_boundary_location(tc,
            character_index_to_ptr(tc, tc.data->cursor_position)),
        make_vector<int>(1, get_text_layout(tc).line_height));
}

// Get the number of the line that the cursor is on.
static size_t
get_cursor_line_number(text_control_parameters const& tc)
{
    return get_line_number(tc, character_index_to_ptr(tc,
        tc.data->cursor_position));
}

// Get the character index that corresponds to the cursor position shifted
// down by delta lines (a negative delta shifts up).
static disambiguated_utf8_offset
get_vertically_adjusted_position(text_control_parameters const& tc, int delta)
{
    size_t line_n = get_cursor_line_number(tc);
    if (tc.data->true_cursor_x < 0)
    {
        tc.data->true_cursor_x =
            get_character_position(get_text_layout(tc),
                character_index_to_ptr(tc, tc.data->cursor_position))[0];
    }

    int adjusted_line_n = int(line_n) + delta;
    if (adjusted_line_n < 0)
        return disambiguated_utf8_offset(0);
    if (adjusted_line_n >= int(get_line_count(tc)))
    {
        return disambiguated_utf8_offset(
            get_text_layout(tc).text.length(), true);
    }

    return character_ptr_to_index(tc,
        get_character_boundary_at_point(
            get_text_layout(tc),
            make_vector<int>(
                tc.data->true_cursor_x,
                get_character_position(
                    get_text_layout(tc),
                    get_line_begin(tc, adjusted_line_n))[1])));
}

// Get the position that the home key should go to.
static disambiguated_utf8_ptr
get_line_home_position(text_control_parameters const& tc)
{
    return get_line_begin(tc, get_cursor_line_number(tc));
}

// Get the position that the end key should go to.
static disambiguated_utf8_ptr
get_line_end_position(text_control_parameters const& tc)
{
    return get_line_end(tc, get_cursor_line_number(tc));
}

static bool is_read_only(text_control_parameters const& tc)
{ return (tc.flags & TEXT_CONTROL_DISABLED) != 0; }

static bool is_disabled(text_control_parameters const& tc)
{ return (tc.flags & TEXT_CONTROL_DISABLED) != 0; }

static bool is_single_line(text_control_parameters const& tc)
{ return (tc.flags & TEXT_CONTROL_SINGLE_LINE) != 0; }

static bool is_multiline(text_control_parameters const& tc)
{ return (tc.flags & TEXT_CONTROL_MULTILINE) != 0; }

static void record_change(text_control_parameters const& tc)
{
    tc.data->change_detected = true;
}

static void ensure_cursor_visible(text_control_parameters const& tc)
{
    make_widget_visible(*tc.ctx, get_cursor_id(tc));
}

// Reset the cursor blink so that it's visible.
static void reset_cursor_blink(text_control_parameters const& tc)
{
    tc.data->cursor_on = true;
    start_timer(*tc.ctx, get_cursor_id(tc), cursor_blink_delay);
}

static void on_text_change(text_control_parameters const& tc)
{
    tc.data->true_cursor_x = -1;
    record_change(tc);
}

static void on_edit(text_control_parameters const& tc)
{
    on_text_change(tc);
    if (tc.flags & TEXT_CONTROL_IMMEDIATE)
    {
        set(*tc.value, tc.data->text);
        tc.result->changed = true;
    }
    else
        tc.data->text_edited = true;
}

static void exit_edit_mode(text_control_parameters const& tc)
{
    tc.data->editing = false;
    tc.data->n_selected = 0;
    tc.data->cursor_on = false;
}

static void reset_to_external_value(text_control_parameters const& tc)
{
    auto new_value = tc.value->is_gettable() ? get(*tc.value) : "";
    // It's possible that we actually caused the change in the external text (e.g., when
    // we're immediately sending out changes), so if we already have the new value, don't
    // actually reset.
    if (tc.data->text == new_value)
        return;
    tc.data->text = new_value;
    tc.data->cursor_position =
        disambiguated_utf8_offset(tc.data->text.length());
    on_text_change(tc);
    tc.data->text_edited = false;
    if (!(tc.flags & TEXT_CONTROL_IMMEDIATE))
        exit_edit_mode(tc);
}

static void do_refresh(text_control_parameters const& tc)
{
    ui_context& ctx = *tc.ctx;
    text_control_data& data = *tc.data;

    if (!data.external_id.matches(tc.value->id()))
    {
        // The value changed through some external program logic,
        // so update the displayed text to reflect it.
        // This also aborts any edits that may have been taking
        // place.
        reset_to_external_value(tc);
        data.external_id.store(tc.value->id());
    }

    if (!data.style_id.matches(*ctx.style.id))
    {
        record_change(tc);
        data.style_id.store(*ctx.style.id);
    }

    if (tc.flags != data.flags)
    {
        record_change(tc);
        data.flags = tc.flags;
    }

    update_layout_cacher(get_layout_traversal(ctx), data.layout_cacher,
        UNPADDED, BASELINE_Y | GROW_X);

    if (data.change_detected)
    {
        ++data.change_counter;
        record_layout_change(get_layout_traversal(ctx));

        data.font = ctx.style.properties->font;
        data.layout_node.set_data(data);

        data.change_detected = false;
    }

    add_layout_node(get_layout_traversal(ctx), &data.layout_node);
}

static void update_text_layout(text_control_parameters const& tc)
{
    ui_context& ctx = *tc.ctx;
    text_control_data& data = *tc.data;

    refresh_keyed_data(data.text_layout,
        combine_ids(make_id(data.change_counter),
            make_id(get_text_region(tc).size[0])));
    if (!is_valid(data.text_layout))
    {
        calculate_text_layout(data.text_layout.value,
            get_display_text(data), ctx.style.properties->font,
            // - 1 to leave room for the cursor
            get_text_region(tc).size[0] - 1,
            // for editing
            true);
        mark_valid(data.text_layout);
    }
}

static void render(text_control_parameters const& tc)
{
    ui_context& ctx = *tc.ctx;
    text_control_data& data = *tc.data;

    if (!is_visible(get_geometry_context(ctx),
            box<2,double>(get_full_region(tc))))
    {
        return;
    }

    refresh_keyed_data(data.unselected_image,
        combine_ids(make_id(data.change_counter),
            make_id(get_text_region(tc).size[0])));
    if (!is_valid(data.unselected_image) ||
        !is_valid(data.unselected_image.value))
    {
        render_text_image(
            *ctx.surface,
            data.unselected_image.value,
            get_text_region(tc).size,
            get_text_layout(tc),
            ctx.style.properties->text_color,
            ctx.style.properties->background_color);
        mark_valid(data.unselected_image);
    }

    if (data.n_selected != 0)
    {
        refresh_keyed_data(data.selected_image,
            combine_ids(make_id(data.change_counter),
                make_id(get_text_region(tc).size[0])));
        if (!is_valid(data.selected_image) ||
            !is_valid(data.selected_image.value))
        {
            render_text_image(
                *ctx.surface,
                data.selected_image.value,
                get_text_region(tc).size,
                get_text_layout(tc),
                get_color_property(ctx, "selected-color"),
                get_color_property(ctx, "selected-background"));
            mark_valid(data.selected_image);
        }

        draw_text_with_selection(
            *ctx.surface,
            get_text_layout(tc),
            data.unselected_image.value, data.selected_image.value,
            get_text_region(tc),
            character_index_to_ptr(tc, data.first_selected),
            character_index_to_ptr(tc, data.first_selected + data.n_selected));
    }
    else
    {
        data.unselected_image.value->draw(
            *ctx.surface,
            box<2,double>(get_text_region(tc)),
            box<2,double>(
                make_vector(0., 0.),
                vector<2,double>(get_text_region(tc).size)));
    }

    if (data.cursor_on && data.editing && data.n_selected == 0)
    {
        vector<2,int> cursor_p =
            get_character_boundary_location(tc,
                character_index_to_ptr(tc, data.cursor_position));
        ctx.surface->draw_filled_box(
            ctx.style.properties->text_color,
            box<2,double>(vector<2,double>(cursor_p),
                make_vector<double>(1, get_text_layout(tc).line_height)));
    }
}

// Call this after any key press.
static void
acknowledge_key(text_control_parameters const& tc)
{
    reset_cursor_blink(tc);
    acknowledge_input_event(*tc.ctx);
    ensure_cursor_visible(tc);
}

// Is there currently any text selected?
static bool has_selection(text_control_parameters const& tc)
{
    text_control_data& data = *tc.data;
    return data.n_selected != 0;
}

// Delete the current selection.
void delete_selection(text_control_parameters const& tc)
{
    text_control_data& data = *tc.data;
    if (has_selection(tc))
    {
        data.text = data.text.substr(0, data.first_selected) +
            data.text.substr(data.first_selected + data.n_selected);
        data.cursor_position = disambiguated_utf8_offset(data.first_selected);
        data.n_selected = 0;
    }
}

// Insert text at the current cursor position.
static void
insert_text(text_control_parameters const& tc, string const& text)
{
    text_control_data& data = *tc.data;
    if (!tc.length_limit ||
        data.text.length() + text.length() - data.n_selected
            <= get(tc.length_limit))
    {
        delete_selection(tc);
        data.text = data.text.substr(0, data.cursor_position.offset) +
            text + data.text.substr(data.cursor_position.offset);
        data.cursor_position.offset += text.length();
    }
}

// Set the current selection.
static void
set_selection(text_control_parameters const& tc, size_t from, size_t to)
{
    text_control_data& data = *tc.data;
    if (from > to)
        std::swap(from, to);
    data.first_selected = from;
    data.n_selected = to - from;
}

// Select all text.
static void
select_all(text_control_parameters const& tc)
{
    text_control_data& data = *tc.data;
    data.first_selected = 0;
    data.n_selected = data.text.length();
    data.cursor_position = disambiguated_utf8_offset(data.n_selected);
}

// Copy the current selection to the clipboard.
static void
copy_selection(text_control_parameters const& tc)
{
    ui_context& ctx = *tc.ctx;
    text_control_data& data = *tc.data;
    if (!(tc.flags & TEXT_CONTROL_MASK_CONTENTS) && has_selection(tc))
    {
        ctx.system->os->set_clipboard_text(
            data.text.substr(data.first_selected, data.n_selected));
    }
}

// Cut the current selection.
static void
cut_selection(text_control_parameters const& tc)
{
    copy_selection(tc);
    delete_selection(tc);
}

// Paste the current clipboard contents into the control.
static void
paste_into(text_control_parameters const& tc)
{
    insert_text(tc, tc.ctx->system->os->get_clipboard_text());
}

// Move the cursor to the given position.
static void
move_cursor(text_control_parameters const& tc,
    disambiguated_utf8_offset const& new_position, bool reset_x = true)
{
    text_control_data& data = *tc.data;
    data.cursor_position = new_position;
    data.n_selected = 0;
    if (reset_x)
        data.true_cursor_x = -1;
}

static disambiguated_utf8_offset
shifted_cursor_position(text_control_parameters const& tc, int shift)
{
    text_control_data& data = *tc.data;
    if (shift < 0)
    {
        return disambiguated_utf8_offset(
            size_t(-shift) > data.cursor_position.offset ?
                0 : data.cursor_position.offset + shift);
    }
    else
    {
        return disambiguated_utf8_offset(
            (std::min)(
                get_text_layout(tc).text.length(),
                data.cursor_position.offset + shift));
    }
}

// Move the cursor, manipulating the selection in the process.
static void
shift_move_cursor(text_control_parameters const& tc,
    disambiguated_utf8_offset const& new_position, bool reset_x = true)
{
    text_control_data& data = *tc.data;

    size_t selection_end = data.first_selected + data.n_selected;

    if (has_selection(tc) &&
        data.cursor_position.offset == data.first_selected)
    {
        set_selection(tc, new_position.offset, selection_end);
    }
    else if (has_selection(tc) &&
        data.cursor_position.offset == selection_end)
    {
        set_selection(tc, data.first_selected, new_position.offset);
    }
    else
        set_selection(tc, data.cursor_position.offset, new_position.offset);

    data.cursor_position = new_position;

    if (reset_x)
        data.true_cursor_x = -1;
}

static void
handle_delete_key(text_control_parameters const& tc)
{
    text_control_data& data = *tc.data;
    if (data.editing)
    {
        if (has_selection(tc))
        {
            delete_selection(tc);
        }
        else if (data.cursor_position.offset < data.text.length())
        {
            data.text =
                data.text.substr(0, data.cursor_position.offset) +
                data.text.substr(data.cursor_position.offset + 1);
        }
        on_edit(tc);
    }
}

static void
handle_key_press(
    text_control_parameters const& tc, key_event_info const & key)
{
    ui_context& ctx = *tc.ctx;
    text_control_data& data = *tc.data;
    switch (key.mods.code)
    {
     case 0:
        switch (key.code)
        {
         case KEY_HOME:
            move_cursor(tc, character_ptr_to_index(tc,
                get_line_home_position(tc)));
            acknowledge_key(tc);
            break;

         case KEY_END:
            move_cursor(tc, character_ptr_to_index(tc,
                get_line_end_position(tc)));
            acknowledge_key(tc);
            break;

         case KEY_ENTER:
            if (data.editing)
            {
                if (is_multiline(tc))
                {
                    insert_text(tc, "\n");
                    on_edit(tc);
                }
                else
                {
                    if (data.text_edited)
                    {
                        set(*tc.value, data.text);
                        if (!tc.validation->error_occurred)
                            reset_to_external_value(tc);
                        tc.result->changed = true;
                    }
                    if (!(tc.flags & TEXT_CONTROL_IMMEDIATE))
                        exit_edit_mode(tc);
                    tc.result->event = TEXT_CONTROL_ENTER_PRESSED;
                }
            }
            else
                data.editing = true;
            acknowledge_key(tc);
            break;

         case KEY_ESCAPE:
            reset_to_external_value(tc);
            tc.result->event = TEXT_CONTROL_EDIT_CANCELED;
            acknowledge_input_event(ctx);
            break;

         case KEY_BACKSPACE:
            if (data.editing)
            {
                if (has_selection(tc))
                {
                    delete_selection(tc);
                }
                else if (data.cursor_position.offset > 0)
                {
                    data.text =
                        data.text.substr(0, data.cursor_position.offset - 1) +
                        data.text.substr(data.cursor_position.offset);
                    data.cursor_position =
                        disambiguated_utf8_offset(
                            data.cursor_position.offset - 1);
                }
                on_edit(tc);
            }
            acknowledge_key(tc);
            break;

         case KEY_DELETE:
            handle_delete_key(tc);
            acknowledge_key(tc);
            break;

         case KEY_LEFT:
            move_cursor(tc, shifted_cursor_position(tc, -1));
            acknowledge_key(tc);
            break;

         case KEY_RIGHT:
            move_cursor(tc, shifted_cursor_position(tc, 1));
            acknowledge_key(tc);
            break;

         case KEY_UP:
            if (is_multiline(tc) || get_line_count(tc) > 1)
            {
                move_cursor(tc, get_vertically_adjusted_position(tc, -1),
                    false);
                acknowledge_key(tc);
            }
            break;

         case KEY_DOWN:
            if (is_multiline(tc) || get_line_count(tc) > 1)
            {
                move_cursor(tc, get_vertically_adjusted_position(tc, 1),
                    false);
                acknowledge_key(tc);
            }
            break;

         case KEY_PAGEUP:
            if (is_multiline(tc) || get_line_count(tc) > 1)
            {
                move_cursor(tc,
                    get_vertically_adjusted_position(tc,
                        -(get_text_region(tc).size[1] /
                        get_text_layout(tc).line_height - 1)),
                    false);
                acknowledge_key(tc);
            }
            break;

         case KEY_PAGEDOWN:
            if (is_multiline(tc) || get_line_count(tc) > 1)
            {
                move_cursor(tc,
                    get_vertically_adjusted_position(tc,
                        get_text_region(tc).size[1] /
                        get_text_layout(tc).line_height - 1),
                    false);
                acknowledge_key(tc);
            }
            break;
        }
        break;

    #ifdef TARGET_OS_MAC
     case KMOD_META_CODE:
        switch (key.code)
        {
         case 'a':
            move_cursor(tc, character_ptr_to_index(tc,
                get_line_home_position(tc)));
            acknowledge_key(tc);
            break;
         case 'e':
            move_cursor(tc, character_ptr_to_index(tc,
                get_line_end_position(tc)));
            acknowledge_key(tc);
            break;
         case 'd':
            handle_delete_key(tc);
            acknowledge_key(tc);
            break;
        }
        break;
    #endif

     case KMOD_CTRL_CODE:
        switch (key.code)
        {
        #if defined(WIN32) || defined(TARGET_OS_MAC)
         case 'a':
            select_all(tc);
            acknowledge_key(tc);
            break;
        #else
         case 'a':
            move_cursor(tc, character_ptr_to_index(tc,
                get_line_home_position(tc)));
            acknowledge_key(tc);
            break;
         case 'e':
            move_cursor(tc, character_ptr_to_index(tc,
                get_line_end_position(tc)));
            acknowledge_key(tc);
            acknowledge_key();
            break;
         case 'd':
            handle_delete_key(tc);
            acknowledge_key(tc);
            break;
        #endif

         case 'c':
         case KEY_INSERT:
            copy_selection(tc);
            acknowledge_key(tc);
            break;

         case 'x':
            if (data.editing)
            {
                cut_selection(tc);
                on_edit(tc);
            }
            acknowledge_key(tc);
            break;

         case 'v':
            if (data.editing)
            {
                paste_into(tc);
                on_edit(tc);
            }
            acknowledge_key(tc);
            break;

         case KEY_HOME:
            move_cursor(tc, disambiguated_utf8_offset(0));
            acknowledge_key(tc);
            break;

         case KEY_END:
            move_cursor(tc,
                disambiguated_utf8_offset(get_text_layout(tc).text.length()));
            acknowledge_key(tc);
            break;

         case KEY_DELETE:
            if (data.editing)
            {
                delete_selection(tc);
                on_edit(tc);
            }
            acknowledge_key(tc);
            break;

         case KEY_LEFT:
            move_cursor(tc, character_ptr_to_index(tc,
                disambiguated_utf8_ptr(find_previous_word_start(
                    as_utf8_string(get_text_layout(tc).text),
                    character_index_to_ptr(tc,
                        data.cursor_position.offset)))));
            acknowledge_key(tc);
            break;

         case KEY_RIGHT:
            move_cursor(tc, character_ptr_to_index(tc,
                disambiguated_utf8_ptr(find_next_word_start(
                    utf8_string(
                        character_index_to_ptr(tc,
                            data.cursor_position.offset),
                        as_utf8_string(get_text_layout(tc).text).end)))));
            acknowledge_key(tc);
            break;
        }
        break;

     case KMOD_SHIFT_CODE:
        switch (key.code)
        {
         case KEY_HOME:
            shift_move_cursor(tc, character_ptr_to_index(tc,
                get_line_home_position(tc)));
            acknowledge_key(tc);
            break;

         case KEY_END:
            shift_move_cursor(tc, character_ptr_to_index(tc,
                get_line_end_position(tc)));
            acknowledge_key(tc);
            break;

         case KEY_INSERT:
            if (data.editing)
            {
                paste_into(tc);
                on_edit(tc);
            }
            acknowledge_key(tc);
            break;

         case KEY_DELETE:
            if (data.editing)
            {
                cut_selection(tc);
                on_edit(tc);
            }
            acknowledge_key(tc);
            break;

         case KEY_LEFT:
            shift_move_cursor(tc, shifted_cursor_position(tc, -1));
            acknowledge_key(tc);
            break;

         case KEY_RIGHT:
            shift_move_cursor(tc, shifted_cursor_position(tc, 1));
            acknowledge_key(tc);
            break;

         case KEY_UP:
            if (is_multiline(tc) || get_line_count(tc) > 1)
            {
                shift_move_cursor(tc, get_vertically_adjusted_position(tc, -1),
                    false);
                acknowledge_key(tc);
            }
            break;

         case KEY_DOWN:
            if (is_multiline(tc) || get_line_count(tc) > 1)
            {
                shift_move_cursor(tc, get_vertically_adjusted_position(tc, 1),
                    false);
                acknowledge_key(tc);
            }
            break;

         case KEY_PAGEUP:
            if (is_multiline(tc) || get_line_count(tc) > 1)
            {
                shift_move_cursor(tc,
                    get_vertically_adjusted_position(tc,
                        -(get_text_region(tc).size[1] /
                        get_text_layout(tc).line_height - 1)),
                    false);
                acknowledge_key(tc);
            }
            break;

         case KEY_PAGEDOWN:
            if (is_multiline(tc) || get_line_count(tc) > 1)
            {
                shift_move_cursor(tc,
                    get_vertically_adjusted_position(tc,
                        get_text_region(tc).size[1] /
                        get_text_layout(tc).line_height - 1),
                    false);
                acknowledge_key(tc);
            }
            break;
        }
        break;

     case KMOD_SHIFT_CODE | KMOD_CTRL_CODE:
        switch (key.code)
        {
         case KEY_HOME:
            shift_move_cursor(tc, disambiguated_utf8_offset(0));
            acknowledge_key(tc);
            break;

         case KEY_END:
            shift_move_cursor(tc,
                disambiguated_utf8_offset(data.text.length(), true));
            acknowledge_key(tc);
            break;

         case KEY_LEFT:
            shift_move_cursor(tc, character_ptr_to_index(tc,
                disambiguated_utf8_ptr(find_previous_word_start(
                    as_utf8_string(get_text_layout(tc).text),
                    character_index_to_ptr(tc,
                        data.cursor_position.offset)))));
            acknowledge_key(tc);
            break;

         case KEY_RIGHT:
            shift_move_cursor(tc, character_ptr_to_index(tc,
                disambiguated_utf8_ptr(find_next_word_start(
                    utf8_string(
                        character_index_to_ptr(tc,
                            data.cursor_position.offset),
                        as_utf8_string(get_text_layout(tc).text).end)))));
            acknowledge_key(tc);
            break;
        }
        break;
    }
}

void do_key_input(text_control_parameters const& tc)
{
    ui_context& ctx = *tc.ctx;
    text_control_data& data = *tc.data;

    if (!is_read_only(tc))
        add_to_focus_order(ctx, tc.id);

    utf8_string text;
    if (detect_text_input(ctx, &text, tc.id))
    {
        // Ignore control characters.
        // TODO: Do this in a more Unicode-aware manner.
        if (text.end != text.begin + 1 || isprint(*text.begin))
        {
            if (data.editing)
            {
                insert_text(tc, string(text.begin, text.end - text.begin));
                on_edit(tc);
                acknowledge_key(tc);
            }
        }
    }
    key_event_info info;
    if (detect_key_press(ctx, &info, tc.id))
        handle_key_press(tc, info);
}

static void do_drag(text_control_parameters const& tc)
{
    ui_context& ctx = *tc.ctx;
    text_control_data& data = *tc.data;
    disambiguated_utf8_offset drag_target =
        character_ptr_to_index(tc, get_character_boundary_at_pixel(tc,
            get_integer_mouse_position(ctx)));
    set_selection(tc, data.drag_start_index, drag_target.offset);
    data.cursor_position = drag_target;
    data.true_cursor_x = -1;
    ensure_cursor_visible(tc);
    reset_cursor_blink(tc);
}

void do_input(text_control_parameters const& tc)
{
    ui_context& ctx = *tc.ctx;
    text_control_data& data = *tc.data;

    if (detect_double_click(ctx, tc.id, LEFT_BUTTON))
    {
        string const& display_text = get_text_layout(tc).text;
        optional<utf8_ptr> character =
            get_character_at_pixel(tc, get_integer_mouse_position(ctx));
        if (character)
        {
            utf8_string word = get_containing_word(
                as_utf8_string(display_text), get(character));
            set_selection(tc, character_ptr_to_index(tc, word.begin),
                character_ptr_to_index(tc, word.end));
            data.cursor_position = character_ptr_to_index(tc,
                disambiguated_utf8_ptr(word.end, true));
            data.true_cursor_x = -1;
            reset_cursor_blink(tc);
        }
    }
    else if (detect_mouse_press(ctx, tc.id, LEFT_BUTTON))
    {
        // This determines if the click is just an initial "move the focus
        // to this control and select its text" click or an actual click
        // to move the cursor and/or drag.
        // If the control already has focus, then all clicks are the latter
        // type. Similarly if the control is read-only. It's less clear
        // what to do for multiline controls (and what constitutes a
        // "multiline" control), so this may have to be revisited.
        if (is_read_only(tc) || get_text_layout(tc).rows.size() > 1 ||
            id_has_focus(ctx, tc.id))
        {
            disambiguated_utf8_offset target =
                character_ptr_to_index(tc, get_character_boundary_at_pixel(tc,
                    get_integer_mouse_position(ctx)));
            data.drag_start_index = target.offset;
            move_cursor(tc, target);
            reset_cursor_blink(tc);
            data.safe_to_drag = true;
            if (!is_read_only(tc))
                data.editing = true;
        }
        else
            data.safe_to_drag = false;
    }
    else if (detect_drag(ctx, tc.id, LEFT_BUTTON) && data.safe_to_drag)
    {
        do_drag(tc);
        start_timer(ctx, tc.id, drag_delay);
    }

    if (detect_timer_event(ctx, tc.id) && is_region_active(ctx, tc.id) &&
        is_mouse_button_pressed(ctx, LEFT_BUTTON))
    {
        do_drag(tc);
        restart_timer(ctx, tc.id, drag_delay);
    }

    //if (detect_click(ctx, tc.id, RIGHT_BUTTON))
    //{
    //    text_control_right_click_menu menu(tc);
    //    ctx.surface->show_popup_menu(&menu);
    //}

    do_key_input(tc);

    {
        if (detect_focus_gain(ctx, tc.id))
        {
            if (!is_read_only(tc))
                data.editing = true;
            reset_cursor_blink(tc);
            ensure_cursor_visible(tc);
            if (!is_read_only(tc) && get_line_count(tc) < 2)
                select_all(tc);
        }
        else if (detect_focus_loss(ctx, tc.id))
        {
            if (data.text_edited)
            {
                set(*tc.value, data.text);
                if (!tc.validation->error_occurred)
                    reset_to_external_value(tc);
                tc.result->changed = true;
                tc.result->event = TEXT_CONTROL_FOCUS_LOST;
            }
            exit_edit_mode(tc);
        }
    }

    if (id_has_focus(ctx, tc.id) && detect_timer_event(ctx, get_cursor_id(tc)))
    {
        data.cursor_on = !data.cursor_on;
        restart_timer(ctx, get_cursor_id(tc), cursor_blink_delay);
    }
}

static text_control_result
do_text_control_pass(
    ui_context& ctx,
    accessor<string> const& value,
    validation_error_handler_data* validation,
    layout const& layout_spec,
    text_control_flag_set flags,
    widget_id id,
    optional<size_t> const& length_limit)
{
    text_control_parameters tc;
    tc.ctx = &ctx;
    tc.value = &value;
    tc.layout_spec = &layout_spec;
    tc.flags = flags;
    tc.length_limit = length_limit;

    text_control_result result;
    result.event = TEXT_CONTROL_NO_EVENT;
    result.changed = false;
    tc.result = &result;

    text_control_data* data;
    get_cached_data(ctx, &data);
    tc.data = data;

    tc.validation = validation;

    init_optional_widget_id(id, &data->flags);
    tc.id = id;

    panel p;
    p.begin(
        ctx, text("control"),
        add_default_alignment(
            add_default_size(layout_spec, width(12, EM)),
            LEFT, BASELINE_Y),
        PANEL_UNSAFE_CLICK_DETECTION,
        id,
        (flags & TEXT_CONTROL_DISABLED) ? WIDGET_DISABLED :
            id_has_focus(ctx, id) ? WIDGET_FOCUSED : WIDGET_NORMAL);
    tc.panel = &p;

    switch (ctx.event->category)
    {
     case REFRESH_CATEGORY:
        do_refresh(tc);
        break;
     case RENDER_CATEGORY:
        update_text_layout(tc);
        render(tc);
        break;
     case REGION_CATEGORY:
        update_text_layout(tc);
        do_box_region(ctx, get_cursor_id(tc), get_cursor_region(tc),
            IBEAM_CURSOR);
        do_box_region(ctx, id, get_full_region(tc), IBEAM_CURSOR);
        break;
     case INPUT_CATEGORY:
        update_text_layout(tc);
        do_input(tc);
        break;
    }

    return result;
}

text_control_result
do_unsafe_text_control(
    ui_context& ctx,
    accessor<string> const& value,
    layout const& layout_spec,
    text_control_flag_set flags,
    widget_id id,
    optional<size_t> const& length_limit)
{
    layout spec = add_default_alignment(layout_spec, FILL_X, BASELINE_Y);
    column_layout c(ctx, spec);

    validation_error_handler_data* validation_data;
    get_data(ctx, &validation_data);

    validation_error_reporting_data* reporting;
    get_data(ctx, &reporting);

    scoped_error_reporting_context reporting_context(ctx, reporting);

    text_control_result result =
        do_text_control_pass(
            ctx,
            make_validation_error_handler(ctx, ref(&value), *validation_data),
            validation_data,
            layout_spec, flags, id, length_limit);
    if (result.event == TEXT_CONTROL_EDIT_CANCELED)
        clear_error(*validation_data);

    do_validation_report(ctx, reporting->reports);

    return result;
}

}
