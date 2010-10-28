#include <alia/cached_ascii_text.hpp>
#include <alia/surface.hpp>
#include <cctype>

namespace alia {

cached_ascii_text::cached_ascii_text(
    surface& surface,
    ascii_text_renderer const& renderer,
    std::string const& text,
    int width,
    unsigned flags)
  : surface_(surface)
{
    unsigned length;
    if ((flags & surface::TRUNCATE) != 0)
    {
        length = calculate_truncation(text, width,
            (flags & surface::GREEDY) != 0);
        calculate_character_widths(renderer, text, length);
    }
    else
    {
        length = text.length();
        calculate_character_widths(renderer, text, length);
        width = calculate_line_spans(text, width);
    }

    font_metrics const& metrics = renderer.get_metrics();

    int text_height = metrics.height;
    create_image(text_image_, vector2i(width + metrics.overhang * 2 + 1,
        text_height * get_line_count() + metrics.row_gap *
        (get_line_count() - 1)));
    alia_foreach_pixel(text_image_.view, uint8, i, i = 0);

    int y = 0;
    for (std::vector<line_span>::const_iterator
        i = line_spans_.begin(); i != line_spans_.end(); ++i)
    {
        line_span const& span = *i;
        int x = 0;
        for (offset j = span.begin; j != span.end; ++j)
        {
            char c = text[j];
            int w = character_widths_[j];
            if (!std::isspace(c) || c == ' ' && j != span.end - 1)
            {
                image_view<uint8> character_image;
                renderer.get_character_image(&character_image, c);
                assert(character_image.size ==
                    vector2i(w + metrics.overhang * 2, text_height));
                vector2i clipped_size;
                clipped_size[0] = (std::min)(character_image.size[0],
                    text_image_.view.size[0] - x);
                clipped_size[1] = (std::min)(character_image.size[1],
                    text_image_.view.size[1] - y);
                alia_foreach_pixel2(
                    subimage(text_image_.view,
                        box2i(point2i(x, y), clipped_size)),
                    uint8, i,
                    subimage(character_image,
                        box2i(point2i(0, 0), clipped_size)),
                    uint8, j,
                    i = (std::max)(i, j));
            }
            x += w;
        }
        y += text_height + metrics.row_gap;
    }

    cached_text::initialize(
        length == text.length() ? text : text.substr(0, length),
        renderer.get_font(),
        text_image_.view.size - vector2i(metrics.overhang * 2, 0),
        metrics);
}

unsigned cached_ascii_text::get_line_count() const
{
    return unsigned(line_spans_.size());
}

cached_ascii_text::offset cached_ascii_text::get_line_begin(
    unsigned line_n) const
{
    return line_spans_[line_n].begin;
}

cached_ascii_text::offset cached_ascii_text::get_line_end(
    unsigned line_n) const
{
    offset p = line_spans_[line_n].end;

    // If a line ends in a space, the space isn't really considered part of
    // the line.
    if (p > line_spans_[line_n].begin && std::isspace(get_text()[p - 1]))
        --p;

    return p;
}

void cached_ascii_text::draw(point2d const& p, rgba8 const& fg) const
{
    if (!display_image_)
    {
        if (text_image_.view.size[0] != 0 && text_image_.view.size[1] != 0)
        {
            surface_.cache_image(display_image_,
                make_interface(text_image_.view), surface::ALPHA_IMAGE);
        }
        else
            return;
    }
    display_image_->draw(p - vector2d(get_metrics().overhang, 0), fg);
}

void cached_ascii_text::draw_subimage(point2d const& p, box2d r,
    rgba8 const& bg, rgba8 const& fg) const
{
    if (r.size[0] == 0 || r.size[1] == 0)
        return;

    if (r.corner[0] + r.size[0] > text_image_.view.size[0])
        r.size[0] = text_image_.view.size[0] - r.corner[0];

    point2d poly[4];
    make_polygon(poly, box2d(p, r.size));
    surface_.draw_filled_polygon(bg, poly, 4);

    display_image_->draw_region(p, r, fg);
}
void cached_ascii_text::draw_subimage(point2d const& p, box2d r,
    rgba8 const& fg) const
{
    if (r.size[0] == 0 || r.size[1] == 0)
        return;

    if (r.corner[0] + r.size[0] > text_image_.view.size[0])
        r.size[0] = text_image_.view.size[0] - r.corner[0];

    display_image_->draw_region(p, r, fg);
}

void cached_ascii_text::draw_with_highlight(
    point2d const& p, rgba8 const& fg,
    rgba8 const& highlight_bg, rgba8 const& highlight_fg,
    offset highlight_begin, offset highlight_end) const
{
    if (!display_image_)
    {
        if (text_image_.view.size[0] != 0 && text_image_.view.size[1] != 0)
        {
            surface_.cache_image(display_image_,
                make_interface(text_image_.view), surface::ALPHA_IMAGE);
        }
        else
            return;
    }

    // Draw the overhang on both sides of the image.
    draw_subimage(p - vector2d(get_metrics().overhang, 0),
        box2d(point2d(0, 0),
            vector2d(get_metrics().overhang, text_image_.view.size[1])),
        fg);
    draw_subimage(p + vector2d(get_size()[0], 0),
        box2d(point2d(text_image_.view.size[0] - get_metrics().overhang, 0),
            vector2d(get_metrics().overhang, text_image_.view.size[1])),
        fg);

    // q is the position on the screen that we're rendering to.
    point2d q = p;
    // u is the position inside the text image that we're rendering from.
    point2d u(get_metrics().overhang, 0);

    std::vector<line_span>::const_iterator i = line_spans_.begin(),
        end_i = line_spans_.end();

    // Draw all the non-highlighted full lines before the highlight as one
    // big subregion.
    int row_spacing = get_metrics().height + get_metrics().row_gap;
    while (i != end_i && i->end < highlight_begin)
        ++i;
    {
        int height = int(i - line_spans_.begin()) * row_spacing;
        draw_subimage(q, box2d(u, vector2d(get_size()[0], height)), fg);
        q[1] += height;
        u[1] += height;
    }

    if (i == end_i)
        return;

    offset char_i = i->begin;

    // Now we're on the line where the highlight starts.

    // First draw all the characters before the highlight.
    if (char_i < highlight_begin)
    {
        int width = 0;
        for (; char_i < highlight_begin; ++char_i)
            width += character_widths_[char_i];

        draw_subimage(q, box2d(u, vector2d(width, get_metrics().height)), fg);
        q[0] += width;
        u[0] += width;
    }

    // Now, draw all the highlighted lines, except the last one.
    while (i->end <= highlight_end)
    {
        int width = 0;
        for (; char_i < i->end; ++char_i)
            width += character_widths_[char_i];

        draw_subimage(q,
            box2d(u, vector2d(width, get_metrics().height)),
            highlight_bg, highlight_fg);

        q[0] = p[0]; q[1] += row_spacing;
        u[0] = get_metrics().overhang; u[1] += row_spacing;

        ++i;
        if (i == end_i)
            return;
        char_i = i->begin;
    }

    // Draw the last highlighted line.
    {
        int width = 0;
        for (; char_i < highlight_end; ++char_i)
            width += character_widths_[char_i];

        draw_subimage(q,
            box2d(u, vector2d(width, get_metrics().height)),
            highlight_bg, highlight_fg);
        q[0] += width;
        u[0] += width;
    }

    // Draw the unhighlighted text after the highlight.
    {
        int width = 0;
        for (; char_i < i->end; ++char_i)
            width += character_widths_[char_i];

        draw_subimage(q, box2d(u, vector2d(width, get_metrics().height)),
            fg);

        q[0] = p[0]; q[1] += row_spacing;
        u[0] = get_metrics().overhang; u[1] += row_spacing;

        ++i;
    }

    // Draw all the non-highlighted full lines after the highlight as one
    // big subregion.
    if (u[1] < get_size()[1])
    {
        draw_subimage(q, box2d(u, vector2d(get_size()[0],
            get_size()[1] - u[1])), fg);
    }
}

void cached_ascii_text::draw_cursor(point2d const& p,
    rgba8 const& color) const
{
    surface_.draw_line(color, line_style(1, solid_line),
        p + vector2d(0.5, 0.5), p + vector2d(0.5, get_metrics().height + 0.5));
}

cached_ascii_text::offset cached_ascii_text::get_character_at_point(
    point2i const& p) const
{
    std::string const& text = get_text();

    int row = p[1] / (get_metrics().height + get_metrics().row_gap);
    if (row < 0)
        return -1;
    if (row >= int(line_spans_.size()))
        return int(text.length());

    line_span const& span = line_spans_[row];

    int x = p[0];
    if (x < 0)
        return -1;

    int accumulated_width = 0;
    for (offset i = span.begin; i < span.end; ++i)
    {
        int char_width = character_widths_[i];
        accumulated_width += char_width;
        if (x < accumulated_width)
            return i;
    }

    return int(text.length());
}

cached_ascii_text::offset cached_ascii_text::get_character_boundary_at_point(
    point2i const& p) const
{
    std::string const& text = get_text();

    int row = p[1] / (get_metrics().height + get_metrics().row_gap);
    if (row < 0)
        return 0;
    if (row >= int(line_spans_.size()))
        return int(text.length());

    line_span const& span = line_spans_[row];

    int x = p[0];
    if (x < 0)
        return span.begin;

    int accumulated_width = 0;
    for (offset i = span.begin; i < span.end; ++i)
    {
        int char_width = character_widths_[i];
        if (x < accumulated_width + char_width / 2)
            return i;
        accumulated_width += char_width;
    }

    return get_line_end(row);
}

unsigned cached_ascii_text::get_line_number(offset position) const
{
    unsigned line_n = 0;
    while (line_n < line_spans_.size() - 1 &&
        line_spans_[line_n].end <= position)
    {
        ++line_n;
    }
    return line_n;
}

point2i cached_ascii_text::get_character_position(offset position) const
{
    unsigned line_n = get_line_number(position);
    int x = 0;
    for (offset i = line_spans_[line_n].begin; i < position; ++i)
        x += character_widths_[i];
    return point2i(x, line_n * (get_metrics().height + get_metrics().row_gap));
}

cached_ascii_text::offset cached_ascii_text::shift(offset position,
    int shift) const
{
    return position + shift;
}

void cached_ascii_text::calculate_character_widths(
    ascii_text_renderer const& renderer, std::string const& text,
    unsigned length)
{
    character_widths_.reset(new int [length]);
    trailing_space_.reset(new int [length]);
    for (unsigned i = 0; i < length; ++i)
    {
        char c = text[i];
        character_widths_[i] = renderer.get_char_width(c);
        trailing_space_[i] = renderer.get_trailing_space(c);
    }
}

int cached_ascii_text::calculate_line_spans(std::string const& text,
    int width)
{
    assert(width >= 0);

    int calculated_width = 0;
    int accumulated_width = 0;
    offset line_start = 0, word_start = 0;
    char last_char = '_';
    offset char_i;
    int trailing_space = 0;
    for (char_i = 0; char_i < offset(text.length()); )
    {
        char this_char = text[char_i];
        if (std::isspace(last_char) && !std::isspace(this_char))
            word_start = char_i;

        // Factor in the trailing space from the last character.
        accumulated_width += trailing_space;
        int char_width = character_widths_[char_i];
        // Record the trailing space from this character, but don't factor it
        // in unless there are more characters on this line.
        trailing_space = trailing_space_[char_i];
        accumulated_width += char_width - trailing_space;

        // A new line is needed when a new-line character is encountered or
        // when the currently line is too big to fit in the window.
        // NOTE: If the line is too big by only a single space, then it is
        // considered OK, because that space will be treated as a new-line
        // anyway.
        if (this_char == '\n'
         || width && accumulated_width > width
         && (std::isspace(last_char) || !std::isspace(this_char)))
        {
            line_span span;
            span.begin = line_start;
            span.end = this_char == '\n' ? char_i + 1 :
                (word_start > line_start ? word_start :
                (char_i > line_start ? char_i : char_i + 1));
            line_spans_.push_back(span);

            line_start = span.end;

            if (!width && accumulated_width > calculated_width)
                calculated_width = accumulated_width;
            accumulated_width = 0;

            char_i = span.end;
            trailing_space = 0;
        }
        else
            ++char_i;

        last_char = this_char;
    }

    if (!width && accumulated_width > calculated_width)
        calculated_width = accumulated_width;

    // Also need to process the end of the last line.
    line_span span;
    span.begin = line_start;
    span.end = char_i;
    line_spans_.push_back(span);

    return width ? width : calculated_width;
}

unsigned cached_ascii_text::calculate_truncation(std::string const& text,
    int width, bool greedy)
{
    assert(width >= 0);

    int accumulated_width = 0;
    offset word_start = 0;
    char last_char = '_';
    offset char_i;
    for (char_i = 0; char_i < offset(text.length()); )
    {
        char this_char = text[char_i];
        if (std::isspace(last_char) && !std::isspace(this_char))
            word_start = char_i;

        int char_width = character_widths_[char_i];
        accumulated_width += char_width;

        if (this_char == '\n'
         || accumulated_width > width
         && (std::isspace(last_char) || !std::isspace(this_char)))
        {
            char_i = this_char == '\n' ? char_i + 1 :
                (word_start || greedy ? word_start :
                (char_i > 0 ? char_i : 1));
            break;
        }
        ++char_i;
        last_char = this_char;
    }

    line_span span;
    span.begin = 0;
    span.end = char_i;
    line_spans_.push_back(span);

    return char_i;
}

}
