#include <alia/ui/utilities/styling.hpp>
#include <alia/ui/utilities.hpp>
#include <sstream>
#include <cctype>

namespace alia {

// STYLE TREE MANIPULATION

void set_style(style_tree& tree, string const& subpath,
    property_map const& properties)
{
    std::size_t first_slash = subpath.find('/');
    if (first_slash == string::npos)
    {
        if (subpath.empty())
            tree.properties = properties;
        else
            tree.substyles[subpath].properties = properties;
        return;
    }

    string child_name = subpath.substr(0, first_slash),
        rest_of_path = subpath.substr(first_slash + 1);

    // This is a little too permissive, since it would accept paths like
    // 'a///b', but there's nothing really wrong with that.
    if (child_name.empty())
    {
        set_style(tree, rest_of_path, properties);
        return;
    }

    set_style(tree.substyles[child_name], rest_of_path, properties);
}

style_tree unflatten_style_tree(flattened_style_tree const& flattened)
{
    style_tree tree;
    for (flattened_style_tree::const_iterator i = flattened.begin();
        i != flattened.end(); ++i)
    {
        set_style(tree, i->first, i->second);
    }
    return tree;
}

string const*
get_style_property(
    style_search_path const* path, char const* property_name)
{
    while (path)
    {
        style_tree const& tree = *path->tree;
        property_map::const_iterator i = tree.properties.find(property_name);
        if (i != tree.properties.end())
            return &i->second;
        path = path->rest;
    }
    return 0;
}

static style_tree const*
find_substyle(
    style_search_path const* path, string const& substyle_name)
{
    while (path)
    {
        style_tree const& tree = *path->tree;
        std::map<string,style_tree>::const_iterator i =
            tree.substyles.find(substyle_name);
        if (i != tree.substyles.end())
            return &i->second;
        path = path->rest;
    }
    return 0;
}

static string widget_state_string(widget_state state)
{
    string s;
    switch (state.code & WIDGET_PRIMARY_STATE_MASK_CODE)
    {
     case WIDGET_DISABLED_CODE:
        s = ".disabled";
        break;
     case WIDGET_HOT_CODE:
        s = ".hot";
        break;
     case WIDGET_DEPRESSED_CODE:
        s = ".depressed";
        break;
     case WIDGET_SELECTED_CODE:
        s = ".selected";
        break;
     case WIDGET_NORMAL_CODE:
     default:
        // the normal state has no specific string associated with it
        ;
    }
    if (state & WIDGET_FOCUSED)
        s += ".focused";
    return s;
}

static style_search_path const*
add_substyle_to_path(style_search_path* storage,
    style_search_path const* path, style_tree const* substyle)
{
    if (substyle)
    {
        storage->tree = substyle;
        storage->rest = path;
        return storage;
    }
    else
        return path;
}

style_search_path const*
add_substyle_to_path(
    style_search_path* storage,
    style_search_path const* search_path,
    style_search_path const* rest,
    string const& substyle_name)
{
    return add_substyle_to_path(storage, rest,
        find_substyle(search_path, substyle_name));
}

style_search_path const*
add_substyle_to_path(
    stateful_style_path_storage* storage,
    style_search_path const* search_path,
    style_search_path const* rest,
    string const& substyle_name,
    widget_state state)
{
    style_search_path const* path;
    
    // Start off with the stateless version as a fallback.
    path = add_substyle_to_path(&storage->nodes[0], rest,
        find_substyle(search_path, substyle_name));

    // If the state has both a primary component and a focused component,
    // try them individually as fallbacks.
    if ((state & WIDGET_FOCUSED) &&
        (state & WIDGET_PRIMARY_STATE_MASK) != WIDGET_NORMAL)
    {
        {
            widget_state substate(state.code & ~WIDGET_FOCUSED_CODE);
            path = add_substyle_to_path(&storage->nodes[1], path,
                find_substyle(search_path,
                    substyle_name + widget_state_string(substate)));
        }
        {
            widget_state substate(
                state.code & ~WIDGET_PRIMARY_STATE_MASK_CODE);
            path = add_substyle_to_path(&storage->nodes[2], path,
                find_substyle(search_path,
                    substyle_name + widget_state_string(substate)));
        }
    }

    // Add the original state itself.
    if (state != WIDGET_NORMAL)
    {
        path = add_substyle_to_path(&storage->nodes[3], path,
            find_substyle(search_path,
                substyle_name + widget_state_string(state)));
    }

    return path;
}

// WHOLE TREE I/O

utf8_ptr skip_space(utf8_string const& text, int& line_count)
{
    utf8_ptr p = text.begin;
    while (p < text.end)
    {
        utf8_ptr q = p;
        SkUnichar c = SkUTF8_NextUnichar(&p);
        if (!is_space(c))
            return q;
        if (is_line_terminator(c))
        {
            ++line_count;
            p = skip_line_terminator(utf8_string(q, text.end));
        }
    }
    return text.end;
}

property_map
parse_style_properties(char const* label, utf8_string const& text,
    utf8_ptr& p, int& line_number)
{
    property_map properties;
    while (1)
    {
        p = skip_space(utf8_string(p, text.end), line_number);

        // Parse the name.
        utf8_ptr name_start = p, name_end;
        bool name_ended = false;
        while (1)
        {
            utf8_ptr q = p;
            SkUnichar c = SkUTF8_NextUnichar(&p);
            if (c == ':')
            {
                name_end = q;
                break;
            }
            if (is_space(c))
            {
                throw parse_error(string(label) + ":" +
                    to_string(line_number) +
                    ": syntax error");
            }
        }
        string name(name_start, name_end - name_start);

        p = skip_space(utf8_string(p, text.end), line_number);

        // Parse the value.
        utf8_ptr value_start = p, value_end;
        while (1)
        {
            utf8_ptr q = p;
            SkUnichar c = SkUTF8_NextUnichar(&p);
            if (c == '}')
            {
                // Don't consume the closing brace.
                p = q;
                value_end = q;
                break;
            }
            if (c == ';')
            {
                value_end = q;
                break;
            }
            if (is_line_terminator(c))
            {
                p = skip_line_terminator(utf8_string(q, text.end));
                ++line_number;
            }
        }
        string value(value_start, value_end - value_start);

        properties[name] = value;

        // Check for a closing brace.
        p = skip_space(utf8_string(p, text.end), line_number);
        SkUnichar c = peek(utf8_string(p, text.end));
        if (c == '}')
        {
            SkUTF8_NextUnichar(&p);
            break;
        }
    }
    return properties;
}

style_tree parse_style_description(char const* label, utf8_string const& text)
{
    style_tree tree;
    int line_number = 1;
    utf8_ptr p = text.begin;
    while (1)
    {
        p = skip_space(utf8_string(p, text.end), line_number);
        if (p == text.end)
            break;

        // Parse the substyle name.
        utf8_ptr next_space = find_next_space(utf8_string(p, text.end));
        string subpath(p, next_space - p);
        p = next_space;
        p = skip_space(utf8_string(p, text.end), line_number);

        // Check for the opening brace of the property map.
        SkUnichar c = SkUTF8_NextUnichar(&p);
        if (c != '{')
        {
            throw parse_error(string(label) + ":" + to_string(line_number) +
                ": syntax error");
        }

        // Parse the property map.
        property_map properties =
            parse_style_properties(label, text, p, line_number);

        // Add the substyle to the tree.
        set_style(tree, subpath, properties);
    }
    return tree;
}

style_tree parse_style_file(char const* path)
{
    FILE* f = fopen(path, "rb");
    if (!f)
        throw parse_error("unable to open file: " + string(path));
    fseek(f, 0, SEEK_END);
    int file_length = ftell(f);
    fseek(f, 0, SEEK_SET);
    // Is it really necessary to terminate the string?
    std::vector<char> text(file_length + 1);
    size_t count = fread(&text[0], 1, file_length, f);
    fclose(f);
    if (count != file_length)
        throw parse_error("unable to read file: " + string(path));
    text[file_length] = '\0';
    return parse_style_description(path,
        utf8_string(&text[0], &text[0] + file_length));
}

static int write_cpp_style_node(FILE* f, style_tree const& node,
    int* index_counter)
{
    int index = *index_counter;
    ++index_counter;
    fprintf(f, "    style_tree node%i;\n", index);
    for (property_map::const_iterator
        i = node.properties.begin(); i != node.properties.end(); ++i)
    {
        fprintf(f, "    node%i.properties[\"%s\"] = \"%s\";\n",
            index, i->first.c_str(), i->second.c_str());
    }
    for (std::map<string,style_tree>::const_iterator
        i = node.substyles.begin(); i != node.substyles.end(); ++i)
    {
        int substyle_index =
            write_cpp_style_node(f, i->second, index_counter);
        fprintf(f, "    node%i.substyles[\"%s\"] = node%i;\n",
            index, i->first.c_str(), substyle_index);
    }
    return index;
}

void write_style_cpp_file(char const* path, char const* label,
    style_tree const& style)
{
    FILE* f = fopen(path, "wb");
    if (!f)
        throw exception("unable to open file: " + string(path));
    fprintf(f, "alia::style_tree %s()\n", label);
    fprintf(f, "{\n");
    int node_n = 0;
    write_cpp_style_node(f, style, &node_n);
    fprintf(f, "    return node0;\n");
    fprintf(f, "}\n");
}

// LINE PARSING

void initialize_line_parser(line_parser& p, char const* text, size_t size)
{
    p.text = text;
    p.text_size = size;
    p.p = text;
}

void initialize_line_parser(line_parser& p, std::string const& text)
{
    initialize_line_parser(p, text.c_str(), text.length());
}

void throw_unexpected_char(line_parser& p)
{
    char buf[64];
    sprintf(buf, "unexpected character: \'%c\' (0x%02x)",
        peek(p), int(peek(p)));
    throw parse_error(buf);
}

void check_char(line_parser& p, char expected)
{
    if (peek(p) != expected)
        throw_unexpected_char(p);
    advance(p);
}

void check_eol(line_parser& p)
{
    if (!is_eol(p))
        throw_unexpected_char(p);
}

bool is_empty(line_parser& p)
{
    skip_space(p);
    return is_eol(p);
}

void check_empty(line_parser& p)
{
    if (!is_empty(p))
        throw_unexpected_char(p);
}

void skip_space(line_parser& p)
{
    while (std::isspace(peek(p)))
        advance(p);
}

void parse(line_parser& p, double* x)
{
    skip_space(p);
    char* end;
    double d = strtod(p.p, &end);
    if (end == p.p)
        throw parse_error("expected number");
    p.p = end;
    *x = d;
}

void parse(line_parser& p, float* x)
{
    double d;
    parse(p, &d);
    *x = float(d);
}

void parse(line_parser& p, int* x)
{
    skip_space(p);
    char* end;
    int i = int(strtol(p.p, &end, 10));
    if (end == p.p)
        throw parse_error("expected integer");
    p.p = end;
    *x = i;
}

// PROPERTY UTILITIES

// colors

void parse(line_parser& p, rgba8* color)
{
    check_char(p, '#');

    uint8_t digits[8];
    int n_digits = 0;
    while (1)
    {
        char c = peek(p);
        uint8_t digit;
        if (c == '\0' || isspace(c))
            break;
        else if (n_digits >= 8)
            throw parse_error("too many digits in color code");
        else if (c >= '0' && c <= '9')
            digit = uint8_t(c - '0');
        else if (c >= 'a' && c <= 'f')
            digit = 10 + uint8_t(c - 'a');
        else if (c >= 'A' && c <= 'F')
            digit = 10 + uint8_t(c - 'A');
        else
            throw_unexpected_char(p);
        digits[n_digits] = digit;
        ++n_digits;
        advance(p);
    }

    switch (n_digits)
    {
     case 3:
        *color = rgba8(
            (digits[0] << 4) + digits[0],
            (digits[1] << 4) + digits[1],
            (digits[2] << 4) + digits[2],
            0xff);
        break;
     case 4:
        *color = rgba8(
            (digits[0] << 4) + digits[0],
            (digits[1] << 4) + digits[1],
            (digits[2] << 4) + digits[2],
            (digits[3] << 4) + digits[3]);
        break;
     case 6:
        *color = rgba8(
            (digits[0] << 4) + digits[1],
            (digits[2] << 4) + digits[3],
            (digits[4] << 4) + digits[5],
            0xff);
        break;
     case 8:
        *color = rgba8(
            (digits[0] << 4) + digits[1],
            (digits[2] << 4) + digits[3],
            (digits[4] << 4) + digits[5],
            (digits[6] << 4) + digits[7]);
        break;
     default:
        throw parse_error("color code digit count is invalid");
    }
}

// CSS-style size specifications

// absolute

void parse(line_parser& p, layout_units* units)
{
    skip_space(p);
    char c0 = peek(p);
    if (c0 == '\0')
        throw parse_error("invalid units");
    advance(p);
    char c1 = peek(p);
    if (c1 == '\0')
        throw parse_error("invalid units");
    advance(p);
    if (!is_eol(p) && !std::isspace(peek(p)))
        throw_unexpected_char(p);
    switch (c0)
    {
     case 'i':
        if (c1 == 'n')
        {
            *units = INCHES;
            return;
        }
        break;
     case 'c':
        if (c1 == 'm')
        {
            *units = CM;
            return;
        }
        break;
     case 'm':
        if (c1 == 'm')
        {
            *units = MM;
            return;
        }
        break;
     case 'e':
        if (c1 == 'm')
        {
            *units = EM;
            return;
        }
        if (c1 == 'x')
        {
            *units = EX;
            return;
        }
        break;
     case 'p':
        if (c1 == 't')
        {
            *units = POINT;
            return;
        }
        if (c1 == 'c')
        {
            *units = PICA;
            return;
        }
        if (c1 == 'x')
        {
            *units = PIXELS;
            return;
        }
        break;
    }
    throw parse_error("invalid units");
}

void parse(line_parser& p, absolute_size_spec* spec)
{
    skip_space(p);
    parse(p, &spec->size);
    skip_space(p);
    parse(p, &spec->units);
}

float
eval_absolute_size_spec(
    ui_context& ctx, unsigned axis, absolute_size_spec const& spec)
{
    return resolve_precise_layout_size(get_layout_traversal(ctx), axis,
        spec.size, spec.units);
}

// relative

void parse(line_parser& p, relative_size_spec* spec)
{
    skip_space(p);
    parse(p, &spec->size);
    skip_space(p);
    if (peek(p) == '%')
    {
        spec->is_relative = true;
        spec->size /= 100;
        advance(p);
        if (!is_eol(p) && !std::isspace(peek(p)))
            throw_unexpected_char(p);
    }
    else
    {
        spec->is_relative = false;
        parse(p, &spec->units);
    }
}

float
eval_relative_size_spec(
    ui_context& ctx, unsigned axis, relative_size_spec const& spec,
    float full_size)
{
    return spec.is_relative ?
        spec.size * full_size :
        resolve_precise_layout_size(get_layout_traversal(ctx), axis,
            spec.size, spec.units);
}

// absolute 2D

void parse(line_parser& p, absolute_size_spec_2d* spec)
{
    parse(p, &spec->axes[0]);
    if (!is_empty(p))
        parse(p, &spec->axes[1]);
    else
        spec->axes[1] = spec->axes[0];
}

vector<2,float>
eval_absolute_size_spec(ui_context& ctx, absolute_size_spec_2d const& spec)
{
    return make_vector(
        eval_absolute_size_spec(ctx, 0, spec.axes[0]),
        eval_absolute_size_spec(ctx, 1, spec.axes[1]));
}

// relative 2D

void parse(line_parser& p, relative_size_spec_2d* spec)
{
    parse(p, &spec->axes[0]);
    if (!is_empty(p))
        parse(p, &spec->axes[1]);
    else
        spec->axes[1] = spec->axes[0];
}

vector<2,float>
eval_relative_size_spec(ui_context& ctx, relative_size_spec_2d const& spec,
    vector<2,float> const& full_size)
{
    return make_vector(
        eval_relative_size_spec(ctx, 0, spec.axes[0], full_size[0]),
        eval_relative_size_spec(ctx, 1, spec.axes[1], full_size[1]));
}

// four corners

static void
fill_in_missing_corners(relative_size_spec* corners, int n_corners)
{
    if (n_corners < 2)
        corners[1] = corners[0];
    if (n_corners < 3)
        corners[2] = corners[0];
    if (n_corners < 4)
        corners[3] = corners[1];
}

void parse(line_parser& p, four_corners_spec* spec)
{
    relative_size_spec specs[2][4];
    int n_specs[2] = { 0, 0 };
    for (int i = 0; i != 2; ++i)
    {
        while (1)
        {
            skip_space(p);
            if (is_eol(p))
                break;
            if (i == 0 && peek(p) == '/')
            {
                advance(p);
                break;
            }
            if (n_specs[i] >= 4)
                throw_unexpected_char(p);
            parse(p, &specs[i][n_specs[i]++]);
        }
    }
    if (n_specs[0] == 0)
        throw parse_error("empty corner list");
    fill_in_missing_corners(specs[0], n_specs[0]);
    if (n_specs[1] == 0)
    {
        for (int i = 0; i != 4; ++i)
            specs[1][i] = specs[0][i];
    }
    else
        fill_in_missing_corners(specs[1], n_specs[1]);
    for (int i = 0; i != 4; ++i)
        spec->corners[i] = relative_size_spec_2d(specs[0][i], specs[1][i]);
}

four_corners_sizes
eval_four_corners(ui_context& ctx, four_corners_spec const& spec,
    vector<2,float> const& full_size)
{
    four_corners_sizes sizes;
    for (int i = 0; i != 4; ++i)
    {
        sizes.corners[i] =
            eval_relative_size_spec(ctx, spec.corners[i], full_size);
    }
    return sizes;
}

// higher-level properties

font get_font_properties(ui_system const& ui, style_search_path const* path)
{
    return font(
        get_property(path, "font-name", string("arial")),
        get_property(path, "font-size", 13.f) * ui.style->text_magnification,
        (get_property(path, "font-bold", false) ? BOLD : NO_FLAGS) |
        (get_property(path, "font-italic", false) ? ITALIC : NO_FLAGS) |
        (get_property(path, "font-underline", false) ? UNDERLINE : NO_FLAGS) |
        (get_property(path, "font-strikethrough", false) ?
            STRIKETHROUGH : NO_FLAGS));
}

void read_primary_style_properties(
    ui_system const& ui,
    primary_style_properties* props,
    style_search_path const* path)
{
    props->text_color =
        get_property(path, "text-color",
            rgba8(0x00, 0x00, 0x00, 0xff));
    props->background_color =
        get_property(path, "background-color",
            rgba8(0xff, 0xff, 0xff, 0xff));
    props->font = get_font_properties(ui, path);
}

void read_layout_style_info(layout_style_info* style_info,
    font const& font, style_search_path const* path)
{
    style_info->is_padded = get_property(path, "padded", false);
    layout_scalar padding_size =
        as_layout_size(get_property(path, "padding-size", 0.2f) *
            font.size);
    style_info->padding_size = make_vector(padding_size, padding_size);

    style_info->font_size = font.size;

    // Skia supposedly supplies all the necessary font metrics, but they're not
    // always valid.
    //SkPaint paint;
    //set_skia_font_info(paint, font);
    //SkPaint::FontMetrics metrics;
    //SkScalar line_spacing = paint.getFontMetrics(&metrics);
    //style_info->character_size = make_vector(
    //    metrics.fAvgCharWidth > 0 ? SkScalarToFloat(metrics.fAvgCharWidth) :
    //        SkScalarToFloat(line_spacing) * 0.6f,
    //    SkScalarToFloat(line_spacing));
    //style_info->x_height =
    //    metrics.fXHeight > 0 ? SkScalarToFloat(metrics.fXHeight) :
    //        SkScalarToFloat(line_spacing) * 0.5f;

    // ... so do some approximations instead.
    style_info->character_size = make_vector(font.size * 0.6f, font.size);
    style_info->x_height = font.size * 0.5f;
}

}
