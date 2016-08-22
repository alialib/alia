#include <alia/ui/utilities/text.hpp>
#include <alia/ui/utilities/skia.hpp>
#include <alia/layout/utilities.hpp>

#include <utf8.h>

namespace alia {

unicode_char_t peek(utf8_string const& text)
{
    return utf8::peek_next(text.begin, text.end);
}

utf8_ptr next_utf8_char(utf8_string const& text)
{
    utf8_ptr p = text.begin;
    utf8::next(p, text.end);
    return p;
}

unicode_char_t static
next_utf8_char(utf8_ptr* start, utf8_ptr const& end)
{
    return utf8::next(*start, end);
}

unicode_char_t static
prev_utf8_char(utf8_ptr* start, utf8_ptr const& end)
{
    return utf8::prior(*start, end);
}

bool is_space(unicode_char_t c)
{
    return
        c >= 0x09 && c <= 0x0d || c == 0x20 || c > 0x80 &&
        (c == 0x85 || c == 0xA0 || c == 0x1680 || c == 0x180E ||
        c >= 0x2000 && c <= 0x200a || c == 0x2028 || c == 0x2029 ||
        c == 0x202f || c == 0x205f || c == 0x3000 || c == 0xfeff);
}

bool is_breakable_space(unicode_char_t c)
{
    return
        c >= 0x09 && c <= 0x0d || c == 0x20 || c > 0x80 &&
        (c == 0x85 || c == 0x1680 || c == 0x180E ||
        c >= 0x2000 && c <= 0x200a || c == 0x2028 || c == 0x2029 ||
        c == 0x205f || c == 0x3000);
}

bool is_line_terminator(unicode_char_t c)
{
    return
        c >= 0x0a && c <= 0x0d || c > 0x80 &&
        (c == 0x85 || c == 0x2028 || c == 0x2029);
}

utf8_ptr skip_line_terminator(utf8_string const& text)
{
    utf8_ptr p = text.begin;
    if (p < text.end)
    {
        unicode_char_t c = next_utf8_char(&p, text.end);
        if (c == 0x0d && p != text.end)
        {
            utf8_ptr q = p;
            unicode_char_t d = next_utf8_char(&p, text.end);
            if (d == 0x0a)
                return p;
            else
                return q;
        }
    }
    return p;
}

utf8_ptr skip_space(utf8_string const& text)
{
    utf8_ptr p = text.begin;
    while (p < text.end)
    {
        utf8_ptr q = p;
        if (!is_space(next_utf8_char(&p, text.end)))
            return q;
    }
    return text.end;
}

utf8_ptr find_next_space(utf8_string const& text)
{
    utf8_ptr p = text.begin;
    while (p < text.end)
    {
        utf8_ptr q = p;
        if (is_space(next_utf8_char(&p, text.end)))
            return q;
    }
    return text.end;
}

utf8_ptr find_next_breakable_space(utf8_string const& text)
{
    utf8_ptr p = text.begin;
    while (p < text.end)
    {
        utf8_ptr q = p;
        if (is_breakable_space(next_utf8_char(&p, text.end)))
            return q;
    }
    return text.end;
}

utf8_ptr find_next_word_start(utf8_string const& text)
{
    utf8_ptr p = find_next_breakable_space(text);
    while (p < text.end)
    {
        utf8_ptr q = p;
        if (!is_space(next_utf8_char(&p, text.end)))
            return q;
    }
    return text.end;
}

utf8_ptr find_previous_word_start(utf8_string const& text, utf8_ptr p)
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
        bool is_space = alia::is_space(prev_utf8_char(&p, text.begin));
        if (is_space && !last_character_was_space)
            return q;
        last_character_was_space = is_space;
    }
    return text.begin;
}

utf8_string get_containing_word(utf8_string const& text, utf8_ptr p)
{
    utf8_ptr q = p;
    bool is_space = alia::is_space(next_utf8_char(&q, text.end));
    // Move q forward to the end of the word.
    while (q < text.end)
    {
        utf8_ptr t = q;
        if (alia::is_space(next_utf8_char(&t, text.end)) != is_space)
            break;
        q = t;
    }
    // Move p backward to the start of the word.
    while (p > text.begin)
    {
        utf8_ptr t = p;
        if (alia::is_space(prev_utf8_char(&t, text.begin)) != is_space)
            break;
        p = t;
    }
    return utf8_string(p, q);
}

utf8_ptr
break_text(
    SkPaint& paint, utf8_string const& text, layout_scalar width,
    bool is_full_line, bool for_editing, layout_scalar* accumulated_width_out,
    layout_scalar* visible_width_out, utf8_ptr* visible_end_out,
    bool* ended_on_line_terminator_out)
{
    utf8_ptr p = text.begin;
    layout_scalar remaining_width = width;
    layout_scalar visible_width = 0;
    utf8_ptr visible_end = p;
    bool ended_on_line_terminator = false;
    while (p < text.end)
    {
        utf8_ptr next_space =
            find_next_breakable_space(utf8_string(p, text.end));
        // Calculate the width up to the next breakable space.
        layout_scalar word_width =
            skia_scalar_as_layout_size(paint.measureText(p, next_space - p));
        // If that width is more than the remaining width on the line, then
        // we need to break.
        if (word_width > remaining_width)
        {
            // If we're at the start of the line, we have a word that's longer
            // than a full line, so fit as much of it as possible.
            if (is_full_line)
            {
                SkScalar measured_width;
                size_t what_will_fit =
                    paint.breakText(p, next_space - p,
                        layout_scalar_as_skia_scalar(remaining_width),
                        &measured_width);
                // Avoid infinite loops!
                if (!what_will_fit)
                    what_will_fit = next_space - p;
                remaining_width -= skia_scalar_as_layout_size(measured_width);
                visible_width = width - remaining_width;
                p += what_will_fit;
                visible_end = p;
            }
            goto line_broken;
        }
        // Advance past that word.
        remaining_width -= word_width;
        visible_width = width - remaining_width;
        visible_end = p = next_space;
        // This block takes care of skipping space at the end of the word.
        // The space is treated differently because it doesn't necessarily
        // need to be visible on the line.
        utf8_ptr space_end = text.end;
        while (p < text.end)
        {
            utf8_ptr q = p;
            unicode_char_t c = next_utf8_char(&p, text.end);
            // If we encounter a line terminator, break the line immediately.
            if (is_line_terminator(c))
            {
                p = skip_line_terminator(utf8_string(q, text.end));
                ended_on_line_terminator = true;
                goto line_broken;
            }
            // If we encounter a non-space character or we're in editing mode
            // and we've already encountered a single space, then we're done.
            if (!is_space(c) ||
                for_editing && q != next_space)
            {
                space_end = q;
                break;
            }
        }
        // Measure the space at the end of the word and advance past it.
        layout_scalar space_width = skia_scalar_as_layout_size(
            paint.measureText(next_space, space_end - next_space));
        p = space_end;
        remaining_width -= space_width;
        // We no longer have a full line.
        is_full_line = false;
    }
 line_broken:
    if (ended_on_line_terminator_out)
        *ended_on_line_terminator_out = ended_on_line_terminator;
    *visible_end_out = visible_end;
    *visible_width_out = visible_width;
    *accumulated_width_out = width - remaining_width;
    return p;
}

// Calculate the length of the longest word (in pixels) in the given text.
layout_scalar get_longest_word(SkPaint& paint, utf8_string const& text)
{
    layout_scalar max_width = as_layout_size(0);
    char const* p = text.begin;
    while (p != text.end)
    {
        utf8_ptr next_space =
            find_next_breakable_space(utf8_string(p, text.end));

        layout_scalar word_width =
            skia_scalar_as_layout_size(paint.measureText(p, next_space - p));
        if (word_width > max_width)
            max_width = word_width;

        p = skip_space(utf8_string(next_space, text.end));
    }
    return max_width;
}

}
