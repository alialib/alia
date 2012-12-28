#include <alia/ui/utilities/text.hpp>
#include <alia/ui/utilities/skia.hpp>

namespace alia {

bool is_breakable_space(SkUnichar c)
{
    return
        c >= 0x09 && c <= 0x0d || c == 0x20 || c > 0x80 &&
        (c == 0x85 || c == 0x1680 || c == 0x180E ||
        c >= 0x2000 && c <= 0x200a || c == 0x2028 || c == 0x2029 ||
        c == 0x205f);
}

bool is_line_terminator(SkUnichar c)
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

utf8_ptr find_next_breakable_space(utf8_string const& text)
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

utf8_ptr find_next_word_start(utf8_string const& text)
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
	bool is_space = is_breakable_space(SkUTF8_PrevUnichar(&p));
        if (is_space && !last_character_was_space)
	    return q;
        last_character_was_space = is_space;
    }
    return text.begin;
}

utf8_string get_containing_word(utf8_string const& text, utf8_ptr p)
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

utf8_ptr
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

}
