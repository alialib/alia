#include <alia/cached_ascii_text.hpp>
#include <alia/rendering.hpp>
#include <cctype>

namespace alia {

void cached_text::initialize(
    string const& text,
    font const& font,
    vector<2,int> const& size,
    font_metrics const& metrics)
{
    text_ = text;
    font_ = font;
    size_ = size;
    metrics_ = metrics;
}

static rgba8 blend_pixels(rgba8 x, rgba8 y, rgba8 bg)
{
    int a = int(x.a) + y.a;
    int r = int(x.r) + y.r;
    int g = int(x.g) + y.g;
    int b = int(x.b) + y.b;
    if (a > 0xff)
    {
        r -= bg.r * (a - 0xff) / 0xff;
        g -= bg.g * (a - 0xff) / 0xff;
        b -= bg.b * (a - 0xff) / 0xff;
    }
    return rgba8(
        r > 0xff ? 0xff : uint8_t(r),
        g > 0xff ? 0xff : uint8_t(g),
        b > 0xff ? 0xff : uint8_t(b),
        a > 0xff ? 0xff : uint8_t(a));
}

static image_view<rgba8>
get_character_image(ascii_font_image const& image, char c)
{
    uint8_t index = uint8_t(c);
    assert(image.advance[index] + image.metrics.overhang * 2 <=
        image.cell_size[0]);
    return subimage(image.image.view, box<2,int>(
        make_vector<int>(
            (index % 16) * image.cell_size[0],
            (index / 16) * image.cell_size[1]),
        make_vector<int>(
            image.advance[index] + image.metrics.overhang * 2,
            image.cell_size[1])));
}

void cached_ascii_text::generate_image(image<rgba8>& text_image, 
    ascii_font_image const& font_image, string const& text, int width)
{
    font_metrics const& metrics = font_image.metrics;

    int text_height = metrics.height;
    create_image(text_image,
        make_vector<int>(
            width + metrics.overhang * 2 + 1,
            text_height * get_line_count() + metrics.row_gap *
                (get_line_count() - 1)));
    alia_foreach_pixel(text_image.view, rgba8, i, i = rgba8(0, 0, 0, 0));

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
                image_view<rgba8> character_image =
                    get_character_image(font_image, c);
                assert(character_image.size ==
                    make_vector<int>(w + metrics.overhang * 2, text_height));
                vector<2,int> clipped_size;
                clipped_size[0] = (std::min)(character_image.size[0],
                    text_image.view.size[0] - x);
                clipped_size[1] = (std::min)(character_image.size[1],
                    text_image.view.size[1] - y);
                alia_foreach_pixel2(
                    subimage(text_image.view,
                        box<2,int>(make_vector<int>(x, y), clipped_size)),
                    rgba8, i,
                    subimage(character_image,
                        box<2,int>(make_vector<int>(0, 0), clipped_size)),
                    rgba8, j,
                    i = blend_pixels(i, j, font_image.bg_color));
            }
            x += w;
        }
        y += text_height + metrics.row_gap;
    }
}

cached_ascii_text::cached_ascii_text(
    ascii_font_image const& font_image,
    ascii_font_image const& highlight_font_image,
    string const& text,
    int width,
    unsigned flags)
{
    font_image_ = &font_image;
    highlight_font_image_ = &highlight_font_image;

    unsigned length;
    if ((flags & TRUNCATE) != 0)
    {
        length = calculate_truncation(text, width,
            (flags & GREEDY) != 0);
        calculate_character_widths(font_image, text, length);
    }
    else
    {
        length = unsigned(text.length());
        calculate_character_widths(font_image, text, length);
        width = calculate_line_spans(text, width);
    }

    font_metrics const& metrics = font_image.metrics;

    generate_image(text_image_, font_image, text, width);

    cached_text::initialize(
        length == text.length() ? text : text.substr(0, length),
        font_image.font,
        text_image_.view.size - make_vector<int>(metrics.overhang * 2, 0),
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

void cached_ascii_text::draw(
    surface& surface, vector<2,double> const& p)
{
    if (!is_valid(display_image_))
    {
        if (text_image_.view.size[0] != 0 && text_image_.view.size[1] != 0)
        {
            surface.cache_image(display_image_,
                make_interface(text_image_.view));
        }
        else
            return;
    }
    display_image_->draw(p - make_vector<double>(get_metrics().overhang, 0));
}

void cached_ascii_text::draw_subimage(
    vector<2,double> const& p, box<2,double> r)
{
    if (r.size[0] == 0 || r.size[1] == 0)
        return;

    if (r.corner[0] + r.size[0] > text_image_.view.size[0])
        r.size[0] = text_image_.view.size[0] - r.corner[0];

    display_image_->draw_region(p, r);
}
void cached_ascii_text::draw_highlight_subimage(
    vector<2,double> const& p, box<2,double> r)
{
    if (r.size[0] == 0 || r.size[1] == 0)
        return;

    if (r.corner[0] + r.size[0] > text_image_.view.size[0])
        r.size[0] = text_image_.view.size[0] - r.corner[0];

    highlight_display_image_->draw_region(p, r);
}

void cached_ascii_text::draw_with_selection(
    surface& surface, vector<2,double> const& p,
    offset highlight_begin, offset highlight_end)
{
    if (!is_valid(display_image_))
    {
        if (text_image_.view.size[0] != 0 && text_image_.view.size[1] != 0)
        {
            surface.cache_image(display_image_,
                make_interface(text_image_.view));
        }
        else
            return;
    }
    if (!is_valid(highlight_display_image_))
    {
        generate_image(highlight_image_, *highlight_font_image_,
            this->get_text(), text_image_.view.size[0]);
        if (highlight_image_.view.size[0] != 0 &&
            highlight_image_.view.size[1] != 0)
        {
            surface.cache_image(highlight_display_image_,
                make_interface(highlight_image_.view));
        }
        else
            return;
    }

    // Draw the overhang on both sides of the image.
    draw_subimage(p - make_vector<double>(get_metrics().overhang, 0),
        box<2,double>(
            make_vector<double>(0, 0),
            make_vector<double>(get_metrics().overhang,
                text_image_.view.size[1])));
    draw_subimage(p + make_vector<double>(get_size()[0], 0),
        box<2,double>(
            make_vector<double>(
                text_image_.view.size[0] - get_metrics().overhang, 0),
            make_vector<double>(
                get_metrics().overhang, text_image_.view.size[1])));

    // q is the position on the screen that we're rendering to.
    vector<2,double> q = p;
    // u is the position inside the text image that we're rendering from.
    vector<2,double> u = make_vector<double>(get_metrics().overhang, 0);

    std::vector<line_span>::const_iterator i = line_spans_.begin(),
        end_i = line_spans_.end();

    // Draw all the non-highlighted full lines before the highlight as one
    // big subregion.
    int row_spacing = get_metrics().height + get_metrics().row_gap;
    while (i != end_i && i->end < highlight_begin)
        ++i;
    {
        int height = int(i - line_spans_.begin()) * row_spacing;
        draw_subimage(q, box<2,double>(u, make_vector<double>(
            get_size()[0], height)));
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

        draw_subimage(q, box<2,double>(u, make_vector<double>(
            width, get_metrics().height)));
        q[0] += width;
        u[0] += width;
    }

    // Now, draw all the highlighted lines, except the last one.
    while (i->end <= highlight_end)
    {
        int width = 0;
        for (; char_i < i->end; ++char_i)
            width += character_widths_[char_i];

        draw_highlight_subimage(q,
            box<2,double>(u,
                make_vector<double>(width, get_metrics().height)));

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

        draw_highlight_subimage(q,
            box<2,double>(u,
                make_vector<double>(width, get_metrics().height)));
        q[0] += width;
        u[0] += width;
    }

    // Draw the unhighlighted text after the highlight.
    {
        int width = 0;
        for (; char_i < i->end; ++char_i)
            width += character_widths_[char_i];

        draw_subimage(q,
            box<2,double>(u,
                make_vector<double>(width, get_metrics().height)));

        q[0] = p[0]; q[1] += row_spacing;
        u[0] = get_metrics().overhang; u[1] += row_spacing;

        ++i;
    }

    // Draw all the non-highlighted full lines after the highlight as one
    // big subregion.
    if (u[1] < get_size()[1])
    {
        draw_subimage(q,
            box<2,double>(u,
                make_vector<double>(get_size()[0], get_size()[1] - u[1])));
    }
}

void cached_ascii_text::draw_cursor(
    surface& surface, bool selected, vector<2,double> const& p)
{
    surface.draw_line(
        selected ? font_image_->text_color :
            highlight_font_image_->text_color,
        line_style(1, solid_line),
        p + make_vector(0.5, 0.5),
        p + make_vector(0.5, get_metrics().height + 0.5));
}

cached_ascii_text::offset cached_ascii_text::get_character_at_point(
    vector<2,int> const& p) const
{
    string const& text = get_text();

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
    vector<2,int> const& p) const
{
    string const& text = get_text();

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

vector<2,int> cached_ascii_text::get_character_position(offset position) const
{
    unsigned line_n = get_line_number(position);
    int x = 0;
    for (offset i = line_spans_[line_n].begin; i < position; ++i)
        x += character_widths_[i];
    return make_vector<int>(x,
        line_n * (get_metrics().height + get_metrics().row_gap));
}

cached_ascii_text::offset cached_ascii_text::shift(offset position,
    int shift) const
{
    return position + shift;
}

void cached_ascii_text::calculate_character_widths(
    ascii_font_image const& font_image, string const& text, unsigned length)
{
    character_widths_.reset(new int [length]);
    trailing_space_.reset(new int [length]);
    for (unsigned i = 0; i < length; ++i)
    {
        char c = text[i];
        character_widths_[i] = font_image.advance[uint8_t(c)];
        trailing_space_[i] = font_image.trailing_space[uint8_t(c)];
    }
}

int cached_ascii_text::calculate_line_spans(string const& text, int width)
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

unsigned cached_ascii_text::calculate_truncation(string const& text,
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
