#include <alia/ascii_text_renderer.hpp>
#include <cctype>

namespace alia {

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

int ascii_text_renderer::compute_advance(utf8_string const& text) const
{
    int width = 0;
    for (utf8_ptr p = text.begin; p != text.end; ++p)
        width += image_->advance[uint8_t(*p)];
    return width;
}

int ascii_text_renderer::measure_text(utf8_string const& text) const
{
    int width = compute_advance(text);
    // Remove the trailing whitespace of the last character.
    if (!is_empty(text))
    {
        width += (std::max)(image_->metrics.overhang * 2,
            image_->advance[' ']);
        // Technically, the trailing whitespace should be removed, but it makes
        // flowing text look more consistent if it's left in.
        //width -= image_->trailing_space[uint8_t(*(text.end - 1))];
    }
    return width;
}

utf8_ptr ascii_text_renderer::break_text(utf8_string const& text, int width,
    bool is_full_line) const
{
    int accumulated_width = 0;
    utf8_ptr word_start = text.begin;
    char last_char = '_'; // initialized to a non-space character
    int trailing_space = 0;
    for (utf8_ptr p = text.begin; p != text.end; ++p)
    {
        char this_char = *p;
        uint8_t char_index = uint8_t(this_char);
        if (std::isspace(last_char) && !std::isspace(this_char))
            word_start = p;

        // Factor in the trailing space from the last character.
        accumulated_width += trailing_space;
        int char_width = image_->advance[char_index];
        // Record the trailing space from this character, but don't factor it
        // in yet.
        trailing_space = image_->trailing_space[char_index];
        accumulated_width += char_width - trailing_space;

        // An explicit new-line character always results in a break.
        // (The new-line character is included in the current line and is
        // considered trailing whitespace.)
        if (this_char == '\n')
            return p + 1;

        // If the line is too big by only a single space, then it is
        // considered OK, because that space won't be displayed.
        if (accumulated_width > width &&
            (std::isspace(last_char) || !std::isspace(this_char)))
        {
            if (word_start > text.begin || !is_full_line)
                return word_start;
            // Ensure that at least one character is included or the caller
            // might end up in an infinite loop.
            return p > text.begin ? p : p + 1;
        }

        last_char = this_char;
    }

    return text.end;
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

void ascii_text_renderer::render_text(
    image_view<rgba8> const& img, utf8_string const& text,
    rgba8 text_color, rgba8 background_color)
{
    font_metrics const& metrics = image_->metrics;

    int text_height = metrics.height;
    alia_foreach_pixel(img, rgba8, i, i = background_color);

    int x = 0;
    for (utf8_ptr p = text.begin; p != text.end; ++p)
    {
        char c = *p;
        uint8_t char_index = uint8_t(c);
        int w = image_->advance[char_index];
        if (!std::isspace(c) || c == ' ' && p != text.end - 1)
        {
            image_view<rgba8> character_image =
                get_character_image(*image_, c);
            assert(character_image.size ==
                make_vector<int>(w + metrics.overhang * 2, text_height));
            vector<2,int> clipped_size;
            clipped_size[0] = (std::min)(character_image.size[0],
                img.size[0] - x);
            clipped_size[1] = (std::min)(character_image.size[1],
                img.size[1]);
            assert(clipped_size[0] >= 0 && clipped_size[1] >= 0);
            alia_foreach_pixel2(
                subimage(img,
                    box<2,int>(make_vector<int>(x, 0), clipped_size)),
                rgba8, i,
                subimage(character_image,
                    box<2,int>(make_vector<int>(0, 0), clipped_size)),
                rgba8, j,
                i = blend_pixels(i, j, image_->bg_color));
            x += w;
        }
    }
}

void draw_ascii_text(
    surface& surface, ascii_font_image const* image,
    cached_image_ptr& display_image, vector<2,double> const& position,
    char const* text, rgba8 const& color)
{
    vector<2,double> p = position;
    //p[0] -= image->metrics.overhang;
    if (!is_valid(display_image))
    {
        surface.cache_image(
            display_image, make_interface(image->image.view));
    }
    for (char const* text_p = text; *text_p; ++text_p)
    {
        uint8_t index = uint8_t(*text_p);
        int character_width = image->advance[index];
        assert(character_width + image->metrics.overhang * 2 <=
            image->cell_size[0]);
        display_image->draw_region(p,
            box<2,double>(
                make_vector<double>(
                    (index % 16) * image->cell_size[0],
                    (index / 16) * image->cell_size[1]),
                make_vector<double>(
                    character_width + image->metrics.overhang * 2,
                    image->cell_size[1])),
            color);
        p[0] += character_width;
    }
}

}
