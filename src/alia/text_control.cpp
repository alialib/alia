#include <alia/text_control.hpp>
#include <alia/context.hpp>
#include <alia/artist.hpp>
#include <alia/input_utils.hpp>
#include <alia/layout.hpp>
#include <alia/panel.hpp>
#include <alia/menu/interface.hpp>
#include <alia/timer.hpp>
#include <cctype>

namespace alia {

namespace impl {

struct text_control_data
{
    text_control_data()
      : assigned_width(0)
      , cursor_on(false)
      , cursor_position(0)
      , editing(false)
      , first_selected(0)
      , n_selected(0)
      , true_cursor_x(-1)
      , need_layout(false)
      , force_cursor_visible(false)
      , text_edited(false)
    {}

    alia::layout_data layout_data;

    int assigned_width;

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
    std::string text;

    // the text representation of the external value
    std::string external_text;

    // true if text is different than external value
    bool text_edited;

    cached_text_ptr renderer;
};

template<typename T>
static T clamp(T x, T min, T max)
{
    assert(min <= max);
    return (std::min)((std::max)(x, min), max);
}

struct text_control
{
 public:
    text_control(
        context& ctx,
        text_control_data& data,
        accessor<std::string> const& value,
        flag_set flags,
        layout const& layout_spec,
        region_id id,
        int max_chars)
      : ctx(ctx), data(data), value(value), artist(*ctx.artist),
        font_metrics(get_font_metrics(ctx, ctx.pass_state.active_font)),
        flags(flags), layout_spec(layout_spec), id(id), max_chars(max_chars)
    {}

    void do_pass()
    {
        result.event = TEXT_CONTROL_NO_EVENT;
        result.changed = false;
        cursor_id = get_region_id(ctx);

        layout spec = layout_spec;
        if ((spec.flags & Y_ALIGNMENT_MASK) == 0)
            spec.flags |= BASELINE_Y;
        if ((spec.flags & X_ALIGNMENT_MASK) == 0)
            spec.flags |= LEFT;

        panel_.begin(ctx, TEXT_CONTROL_STYLE, ROW_LAYOUT, spec);

        switch (ctx.event->category)
        {
         case LAYOUT_CATEGORY:
            do_layout();
            break;
         case RENDER_CATEGORY:
            render();
            break;
         case REGION_CATEGORY:
            do_region(ctx, cursor_id, get_cursor_region());
            do_region(ctx, id, get_full_region());
            break;
         case INPUT_CATEGORY:
            do_input();
            break;
        }
    }

    text_control_result<std::string> result;

 private:
    bool is_password() const { return (flags & PASSWORD) != 0; }
    bool is_read_only() const { return (flags & DISABLED) != 0; }
    bool is_disabled() const { return (flags & DISABLED) != 0; }
    bool is_single_line() const { return (flags & SINGLE_LINE) != 0; }
    bool is_multiline() const { return (flags & MULTILINE) != 0; }

    box2i get_full_region()
    {
        return panel_.get_region();
    }

    box2i get_cursor_region()
    {
        return box2i(
            get_character_boundary_location(data.cursor_position),
            vector2i(1, font_metrics.height));
    }

    std::string get_display_text()
    {
        if (is_password())
            return std::string(data.text.length(), '*');
        else
            return data.text;
    }

    void reset_to_external_value()
    {
        data.text = value.is_valid() ? value.get() : "";
        data.cursor_position = cached_text::offset(data.text.length());
        on_text_change();
        data.text_edited = false;
        if ((flags & ALWAYS_EDITING) == 0)
            exit_edit_mode();
    }

    void do_layout()
    {
        layout spec(BASELINE_Y | GROW_X);
        // All the +/- 1's are to make room for the cursor.
        int minimum_width = get_font_metrics(ctx,
            ctx.pass_state.active_font).average_width * 6 + 1;
        int default_width = get_font_metrics(ctx,
            ctx.pass_state.active_font).average_width * 12 + 1;
        // TODO: The user specified size should probably only be used
        // here if it's in character-related units.
        vector2i requested_size = resolve_size(ctx, layout_spec.size);
        if (requested_size[0] > 0)
            ++requested_size[0];
        switch (ctx.event->type)
        {
         case REFRESH_EVENT:
            layout_widget(ctx, data.layout_data, spec, requested_size,
                widget_layout_info(vector2i(minimum_width, 0), 0, 0,
                    vector2i(default_width, 0), TOP | FILL_X, true));
            if (data.external_text != (value.is_valid() ? value.get() : ""))
            {
                // The value changed through some external program logic,
                // so update the displayed text to reflect it.
                // This also aborts any edits that may have been taking
                // place.
                data.external_text = value.is_valid() ? value.get() : "";
                reset_to_external_value();
            }
            if (!data.renderer || data.need_layout ||
                ctx.pass_state.active_font != data.renderer->get_font())
            {
                data.renderer.reset();
                record_layout_change(ctx, data.layout_data);
                data.need_layout = false;
            }
            break;
         case LAYOUT_PASS_0:
            layout_widget(ctx, data.layout_data, spec, requested_size,
                widget_layout_info(vector2i(minimum_width, 0), 0, 0,
                    vector2i(default_width, 0), TOP | FILL_X, true));
            break;
         case LAYOUT_PASS_1:
            if (get_event<layout_event>(ctx).active_logic)
            {
              {
                resolved_layout_spec resolved;
                resolve_layout_spec(ctx, &resolved, spec, requested_size,
                    widget_layout_info(vector2i(minimum_width, 0), 0, 0,
                        vector2i(default_width, 0), TOP | FILL_X, true));
                int assigned_width = get_assigned_width(ctx, resolved);
                if (!data.renderer ||
                    assigned_width - 1 != data.renderer->get_size()[0])
                {
                    ctx.surface->cache_text(data.renderer,
                        ctx.pass_state.active_font, get_display_text().c_str(),
                        assigned_width - 1);
                    data.assigned_width = assigned_width;
                }
              }
              {
                resolved_layout_spec resolved;
                resolve_layout_spec(ctx, &resolved, spec, requested_size,
                    widget_layout_info(
                        vector2i(minimum_width, data.renderer->get_size()[1]),
                        data.renderer->get_metrics().ascent,
                        data.renderer->get_size()[1] -
                            data.renderer->get_metrics().ascent,
                        vector2i(default_width, data.renderer->get_size()[1]),
                        TOP | FILL_X, true));
                request_vertical_space(ctx, resolved);
              }
            }
            break;
         case LAYOUT_PASS_2:
            if (get_event<layout_event>(ctx).active_logic)
            {
              {
                resolved_layout_spec resolved;
                resolve_layout_spec(ctx, &resolved, spec, requested_size,
                    widget_layout_info(
                        vector2i(minimum_width, data.renderer->get_size()[1]),
                        data.renderer->get_metrics().ascent,
                        data.renderer->get_size()[1] -
                            data.renderer->get_metrics().ascent,
                        vector2i(default_width, data.renderer->get_size()[1]),
                        TOP | FILL_X, true));
                get_assigned_region(ctx, &data.layout_data.assigned_region,
                    resolved);
              }
              {
                // Need to record the same spec as will be used in the REFRESH
                // pass.
                resolved_layout_spec resolved;
                resolve_layout_spec(ctx, &resolved, spec, requested_size,
                    widget_layout_info(vector2i(minimum_width, 0), 0, 0,
                        vector2i(default_width, 0), TOP | FILL_X, true));
                record_layout(ctx, data.layout_data, resolved);
              }
                if (data.force_cursor_visible)
                    ensure_cursor_visible();
            }
            break;
        }
    }

    void render()
    {
        if (data.n_selected != 0)
        {
            data.renderer->draw_with_highlight(
                point2d(data.layout_data.assigned_region.corner),
                ctx.pass_state.text_color,
                ctx.pass_state.selected_bg_color,
                ctx.pass_state.selected_text_color,
                data.first_selected,
                data.first_selected + data.n_selected);
        }
        else
        {
            data.renderer->draw(
                point2d(data.layout_data.assigned_region.corner),
                ctx.pass_state.text_color);
        }

        if (data.cursor_on && data.editing)
        {
            point2i cursor_p = get_character_boundary_location(
                data.cursor_position);
            rgba8 const& cursor_color =
                (data.n_selected != 0 &&
                data.cursor_position >= data.first_selected &&
                data.cursor_position <
                    cached_text::offset(data.first_selected + data.n_selected))
              ? ctx.pass_state.selected_text_color
              : ctx.pass_state.text_color;
            data.renderer->draw_cursor(point2d(cursor_p), cursor_color);
        }

        if (is_region_hot(ctx, id) || detect_drag(ctx, id, LEFT_BUTTON))
            ctx.surface->set_mouse_cursor(IBEAM_CURSOR);
    }

    void do_input()
    {
        if (detect_double_click(ctx, id, LEFT_BUTTON))
        {
            std::string display_text = data.renderer->get_text();
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
        else if (detect_mouse_down(ctx, id, LEFT_BUTTON))
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

        if (detect_click(ctx, id, RIGHT_BUTTON))
        {
            right_click_menu menu(*this);
            ctx.surface->show_popup_menu(&menu);
        }

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
                    data.external_text = data.text;
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

        int c = detect_char(ctx, id);
        if (c > 0 && c <= 0xff && std::isprint(c))
        {
            if (data.editing)
            {
                insert_text(std::string(1, c));
                on_edit();
            }
            acknowledge_key();
        }
        key_event_info info;
        if (detect_key_press(ctx, &info, id))
        {
            switch (info.mods)
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
                                data.external_text = data.text;
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

                 case KEY_BACK:
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
                            -(data.layout_data.assigned_region.size[1] /
                            font_metrics.height - 1)), false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEDOWN:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        move_cursor(get_vertically_adjusted_position(
                            data.layout_data.assigned_region.size[1] /
                            font_metrics.height - 1), false);
                        acknowledge_key();
                    }
                    break;

                 default:
                    ;
                }
                break;

             case MOD_CONTROL:
                switch (info.code)
                {
                 case 'A':
                    select_all();
                    acknowledge_key();
                    break;

                 case 'C':
                 case KEY_INSERT:
                    copy();
                    acknowledge_key();
                    break;

                 case 'X':
                    if (data.editing)
                    {
                        cut();
                        on_edit();
                    }
                    acknowledge_key();
                    break;

                 case 'V':
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

             case MOD_SHIFT:
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
                            -(data.layout_data.assigned_region.size[1] /
                            font_metrics.height - 1)), false);
                        acknowledge_key();
                    }
                    break;

                 case KEY_PAGEDOWN:
                    if (is_multiline() || data.renderer->get_line_count() > 1)
                    {
                        shift_move_cursor(get_vertically_adjusted_position(
                            data.layout_data.assigned_region.size[1] /
                            font_metrics.height - 1), false);
                        acknowledge_key();
                    }
                    break;

                 default:
                    ;
                }
                break;

             case MOD_SHIFT | MOD_CONTROL:
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
        make_region_visible(ctx, cursor_id);
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

    std::string sanitize(std::string const& text)
    {
        std::string r;
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
    void insert_text(std::string const& text)
    {
        std::string sanitized = sanitize(text);
        if (max_chars < 0 ||
            int(data.text.length() + sanitized.length() - data.n_selected)
            <= max_chars)
        {
            delete_selection();
            data.text = data.text.substr(0, data.cursor_position) +
                sanitized + data.text.substr(data.cursor_position);
            data.cursor_position += cached_text::offset(sanitized.length());
        }
        else
            ctx.surface->beep();
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
        return data.renderer->get_character_boundary_at_point(point2i(
            data.true_cursor_x,
            data.renderer->get_character_position(get_line_begin(line_n))[1]));
    }

    // Get the index of the character that contains the given pixel.
    // Will return invalid character indices if the pixel is not actually
    // inside a character.
    int get_character_at_pixel(point2i const& p)
    {
        return data.renderer->get_character_at_point(
            point2i(p - data.layout_data.assigned_region.corner));
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
    int get_character_boundary_at_pixel(point2i const& p)
    {
        return data.renderer->get_character_boundary_at_point(
            point2i(p - data.layout_data.assigned_region.corner));
    }

    // Get the screen location of the character boundary immediately before the
    // given character index.
    point2i get_character_boundary_location(cached_text::offset char_i)
    {
        return data.renderer->get_character_position(char_i) +
            vector2i(data.layout_data.assigned_region.corner);
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

    struct right_click_menu : menu_controller
    {
        right_click_menu(text_control& tc) : tc(tc) {}
        text_control& tc;
        void do_menu(menu_context& ctx)
        {
            if (do_option(ctx, "Cu&t",
                    !tc.is_disabled() && tc.has_selection()))
            {
                tc.cut();
                tc.on_edit();
            }
            if (do_option(ctx, "&Copy", tc.has_selection()))
                tc.copy();
            if (do_option(ctx, "&Paste", !tc.is_disabled()))
            {
                tc.paste();
                tc.on_edit();
                tc.ensure_cursor_visible();
            }
            if (do_option(ctx, "&Delete",
                    !tc.is_disabled() && tc.has_selection()))
            {
                tc.delete_selection();
                tc.on_edit();
            }
            do_separator(ctx);
            if (do_option(ctx, "Select &All"))
            {
                tc.select_all();
                tc.ensure_cursor_visible();
            }
        }
    };

    context& ctx;
    text_control_data& data;
    accessor<std::string> const& value;
    alia::artist& artist;
    alia::font_metrics const& font_metrics;
    flag_set flags;
    layout const& layout_spec;
    region_id id, cursor_id;
    int max_chars;
    static int const cursor_blink_delay = 500;
    static int const drag_delay = 40;
    panel panel_;
};

}

text_control_result<std::string> do_text_control(
    context& ctx,
    accessor<std::string> const& value,
    flag_set flags,
    layout const& layout_spec,
    region_id id,
    int max_chars)
{
    if (!id) id = get_region_id(ctx);
    impl::text_control tc(ctx, *get_data<impl::text_control_data>(ctx), value,
        flags, layout_spec, id, max_chars);
    tc.do_pass();
    return tc.result;
}

}
