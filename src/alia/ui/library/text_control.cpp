#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

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
        if (line_end == p)
        {
            // Nothing is fitting, so we're in an infinite loop. Just abort.
            break;
        }
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
    // beginning of the next line, so we have to avoid returning that position.
    if (!is_empty(row_text) && boundary_before == row_text.end &&
        row_text.end != as_utf8_string(layout.text).end)
    {
        SkUTF8_PrevUnichar(&boundary_before);
        return boundary_before;
    }

    if (boundary_before == row_text.end)
        return row_text.end;

    utf8_ptr boundary_after = boundary_before;
    SkUTF8_NextUnichar(&boundary_after);

    // As above, avoid returning the end of the row's text.
    if (boundary_after == row_text.end &&
        row_text.end != as_utf8_string(layout.text).end)
    {
        return boundary_before;
    }

    SkScalar width_of_character =
        paint.measureText(boundary_before,
            boundary_after - boundary_before);

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

    void set_data(text_control_data& data) { data_ = &data; }

 private:
    text_control_data* data_;
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
};

string get_display_text(text_control_data& tc)
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
            if (line_end == p)
            {
                // Nothing is fitting, so we're in an infinite loop.
                // Just abort.
                break;
            }
            p = line_end;
        }
        while (p != text.end);

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

struct text_control
{
 public:
    text_control(
        ui_context& ctx,
        text_control_data& data,
        accessor<string> const& value,
        layout const& layout_spec,
        text_control_flag_set flags,
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
                add_default_size(layout_spec, width(12, EM)),
                LEFT, BASELINE_Y),
            NO_FLAGS,
            id,
            id_has_focus(ctx, id) ? WIDGET_FOCUSED : WIDGET_NORMAL);

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

    text_control_result result;

 private:
    bool is_read_only() const
    { return (flags & TEXT_CONTROL_DISABLED) != 0; }
    bool is_disabled() const
    { return (flags & TEXT_CONTROL_DISABLED) != 0; }
    bool is_single_line() const
    { return (flags & TEXT_CONTROL_SINGLE_LINE) != 0; }
    bool is_multiline() const
    { return (flags & TEXT_CONTROL_MULTILINE) != 0; }

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
        return get_assignment(data.layout_cacher).region;
    }

    void reset_to_external_value()
    {
        data.text = value.is_gettable() ? get(value) : "";
        data.cursor_position = data.text.length();
        on_text_change();
        data.text_edited = false;
        if (!(flags & TEXT_CONTROL_IMMEDIATE))
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
            data.layout_node.set_data(data);

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
                ctx.style.properties->background_color);
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
                    get_color_property(ctx, "selected-color"),
                    get_color_property(ctx, "selected-background"));
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

        if (data.cursor_on && data.editing && data.n_selected == 0)
        {
            vector<2,int> cursor_p =
                get_character_boundary_location(character_index_to_ptr(
                    data.cursor_position));
            ctx.surface->draw_filled_box(
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

        if (detect_timer_event(ctx, id) && is_region_active(ctx, id) &&
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
                    result.event = TEXT_CONTROL_FOCUS_LOST;
                    result.changed = true;
                }
                exit_edit_mode();
            }
        }

        if (id_has_focus(ctx, id) && detect_timer_event(ctx, cursor_id))
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

    void handle_delete_key()
    {
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
                                result.changed = true;
                            }
                            if (!(flags & TEXT_CONTROL_IMMEDIATE))
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
                    handle_delete_key();
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

            #ifdef TARGET_OS_MAC
             case KMOD_META_CODE:
                switch (info.code)
                {
                 case 'a':
                    move_cursor(character_ptr_to_index(get_home_position()));
                    acknowledge_key();
                    break;
                 case 'e':
                    move_cursor(character_ptr_to_index(get_end_position()));
                    acknowledge_key();
                    break;
                 case 'd':
                    handle_delete_key();
                    acknowledge_key();
                    break;
                }
                break;
            #endif

             case KMOD_CTRL_CODE:
                switch (info.code)
                {
                #if defined(WIN32) || defined(TARGET_OS_MAC)
                 case 'a':
                    select_all();
                    acknowledge_key();
                    break;
                #else
                 case 'a':
                    move_cursor(character_ptr_to_index(get_home_position()));
                    acknowledge_key();
                    break;
                 case 'e':
                    move_cursor(character_ptr_to_index(get_end_position()));
                    acknowledge_key();
                    break;
                 case 'd':
                    handle_delete_key();
                    acknowledge_key();
                    break;
                #endif

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
        if (flags & TEXT_CONTROL_IMMEDIATE)
        {
            value.set(data.text);
            result.changed = true;
        }
        else
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
        if (!(flags & TEXT_CONTROL_MASK_CONTENTS) && has_selection())
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

    // Get the screen location of the character boundary immediately before
    // the given character index.
    vector<2,int> get_character_boundary_location(utf8_ptr character) const
    {
        return get_character_position(get_text_layout(), character) +
            vector<2,int>(get_text_region().corner);
    }

    ui_context& ctx;
    text_control_data& data;
    accessor<string> const& value;
    text_control_flag_set flags;
    layout const& layout_spec;
    widget_id id, cursor_id;
    optional<size_t> length_limit;
    static int const cursor_blink_delay = 500;
    static int const drag_delay = 40;
    panel panel_;
};

static text_control_result
do_text_control_impl(
    ui_context& ctx,
    accessor<string> const& value,
    layout const& layout_spec,
    text_control_flag_set flags,
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

struct text_control_string_conversion
{
    text_control_string_conversion() : valid(false) {}
    bool valid;
    owned_id id;
    string text;
    // associated error message (if text doesn't parse)
    string message;
};

text_control_result
do_text_control(
    ui_context& ctx,
    accessor<string> const& value,
    layout const& layout_spec,
    text_control_flag_set flags,
    widget_id id,
    optional<size_t> const& length_limit)
{
    layout spec = add_default_alignment(layout_spec, FILL_X, BASELINE_Y);
    column_layout c(ctx, spec);

    text_control_string_conversion* data;
    get_data(ctx, &data);
    if (is_refresh_pass(ctx))
    {
        bool valid = value.is_gettable();
        if (data->valid != valid || valid && !data->id.matches(value.id()))
        {
            // The external value has changed.
            data->valid = valid;
            data->text = valid ? get(value) : "";
            data->message = "";
            data->id.store(value.id());
        }
    }

    text_control_result r = do_text_control_impl(
        ctx, inout(&data->text), layout_spec, flags, id, length_limit);
    alia_if(!data->message.empty())
    {
        do_paragraph(ctx, in(data->message));
    }
    alia_end

    text_control_result result;
    switch (r.event)
    {
     case TEXT_CONTROL_FOCUS_LOST:
     case TEXT_CONTROL_ENTER_PRESSED:
      {
        try
        {
            value.set(data->text);
            data->message = "";
            result.event = r.event;
            result.changed = true;
        }
        catch (validation_error& e)
        {
            data->message = e.what();
            result.event = TEXT_CONTROL_INVALID_VALUE;
            result.changed = false;
        }
        break;
      }
     case TEXT_CONTROL_EDIT_CANCELED:
        result.event = TEXT_CONTROL_EDIT_CANCELED;
        result.changed = false;
        data->text = value.is_gettable() ? get(value) : "";
        data->message = "";
        break;
     case TEXT_CONTROL_NO_EVENT:
     default:
        result.event = TEXT_CONTROL_NO_EVENT;
        result.changed = false;
        break;
    }
    return result;
}

}
