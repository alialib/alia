#include <alia/ui/utilities/styling.hpp>
#include <alia/ui/utilities.hpp>
#include <sstream>
#include <cctype>
#include <utf8.h>

namespace alia {

// STYLE TREE MANIPULATION

static style_tree*
get_style_tree_child(style_tree& tree, string const& child_name,
    bool create_if_missing)
{
    if (create_if_missing)
    {
        style_tree_ptr& child = tree.substyles[child_name];
        if (!child)
            child.reset(new style_tree);
        return child.get();
    }
    else
    {
        std::map<string,style_tree_ptr>::iterator child =
            tree.substyles.find(child_name);
        return child != tree.substyles.end() ? child->second.get() : 0;
    }
}

static style_tree*
find_style_node(style_tree& tree, string const& subpath,
    bool create_if_missing)
{
    std::size_t first_slash = subpath.find('/');
    if (first_slash == string::npos)
    {
        if (subpath.empty())
            return &tree;
        else
            return get_style_tree_child(tree, subpath, create_if_missing);
    }

    string child_name = subpath.substr(0, first_slash),
        rest_of_path = subpath.substr(first_slash + 1);

    // This is a little too permissive, since it would accept paths like
    // 'a///b', but there's nothing really wrong with that.
    if (child_name.empty())
        return find_style_node(tree, rest_of_path, create_if_missing);

    style_tree* child = get_style_tree_child(tree, child_name, true);

    return child ? find_style_node(*child, rest_of_path, true) : 0;
}

static std::list<style_tree*>
resolve_flattened_fallbacks(
    style_tree& tree, std::list<string> const& flattened)
{
    std::list<style_tree*> fallbacks;
    for (std::list<string>::const_iterator
        i = flattened.begin(); i != flattened.end(); ++i)
    {
        style_tree* node = find_style_node(tree, *i, false);
        if (!node)
            throw exception("style not found: " + *i);
        fallbacks.push_back(node);
    }
    return fallbacks;
}

void set_style(style_tree& tree, string const& subpath,
    flattened_style_node const& flattened)
{
    std::list<style_tree*> fallbacks =
        resolve_flattened_fallbacks(tree, flattened.fallbacks);
    style_tree* node = find_style_node(tree, subpath, true);
    node->properties = flattened.properties;
    swap(node->fallbacks, fallbacks);
}

style_tree_ptr unflatten_style_tree(flattened_style_tree const& flattened)
{
    style_tree_ptr tree;
    tree.reset(new style_tree);
    for (flattened_style_tree::const_iterator i = flattened.begin();
        i != flattened.end(); ++i)
    {
        set_style(*tree, i->first, i->second);
    }
    return tree;
}

static string const*
get_style_property(style_tree const& tree, char const* property_name)
{
    property_map::const_iterator i = tree.properties.find(property_name);
    if (i != tree.properties.end())
        return &i->second;

    for (std::list<style_tree*>::const_iterator
        i = tree.fallbacks.begin(); i != tree.fallbacks.end(); ++i)
    {
        string const* property = get_style_property(**i, property_name);
        if (property)
            return property;
    }

    return 0;
}

string const*
get_style_property(
    style_search_path const* path, char const* property_name,
    style_search_flag_set flags)
{
    for (; path; path = path->rest)
    {
        if (!path->tree)
        {
            if (flags & INHERITED_PROPERTY)
                continue;
            else
                break;
        }
        string const* property =
            get_style_property(*path->tree, property_name);
        if (property)
            return property;
    }
    return 0;
}

static style_tree const*
find_substyle(
    style_tree const& tree, string const& substyle_name)
{
    std::map<string,style_tree_ptr>::const_iterator i =
        tree.substyles.find(substyle_name);
    if (i != tree.substyles.end())
        return i->second.get();
    for (std::list<style_tree*>::const_iterator
        i = tree.fallbacks.begin(); i != tree.fallbacks.end(); ++i)
    {
        style_tree const* tree = find_substyle(**i, substyle_name);
        if (tree)
            return tree;
    }
    return 0;
}

static style_tree const*
find_substyle(
    style_search_path const* path, string const& substyle_name)
{
    for (; path; path = path->rest)
    {
        if (path->tree)
        {
            style_tree const* tree = find_substyle(*path->tree, substyle_name);
            if (tree)
                return tree;
        }
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
        s = ".normal";
        break;
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

static style_search_path const*
add_path_separator(style_search_path* storage, style_search_path const* path)
{
    storage->tree = 0;
    storage->rest = path;
    return storage;
}

style_search_path const*
add_substyle_to_path(
    style_path_storage* storage,
    style_search_path const* search_path,
    style_search_path const* rest,
    string const& substyle_name,
    add_substyle_flag_set flags)
{
    style_tree const* substyle = find_substyle(search_path, substyle_name);
    return
        (substyle || !(flags & ADD_SUBSTYLE_IFF_EXISTS))
          ? add_substyle_to_path(&storage->nodes[1],
                (flags & ADD_SUBSTYLE_NO_PATH_SEPARATOR) ?
                    rest : add_path_separator(&storage->nodes[0], rest),
                substyle)
          : rest;
}

style_search_path const*
add_substyle_to_path(
    stateful_style_path_storage* storage,
    style_search_path const* search_path,
    style_search_path const* rest,
    string const& substyle_name,
    widget_state state,
    add_substyle_flag_set flags)
{
    style_search_path const* path;

    // Start off with the stateless version as a fallback.
    path = add_substyle_to_path(&storage->nodes[1],
        (flags & ADD_SUBSTYLE_NO_PATH_SEPARATOR) ?
            rest : add_path_separator(&storage->nodes[0], rest),
        find_substyle(search_path, substyle_name));

    // If the state has multiple components, try them individually as
    // fallbacks.
    if (state & WIDGET_FOCUSED)
    {
        {
            widget_state substate(state.code & ~WIDGET_FOCUSED_CODE);
            path = add_substyle_to_path(&storage->nodes[2], path,
                find_substyle(search_path,
                    substyle_name + widget_state_string(substate)));
        }
        if ((state & WIDGET_PRIMARY_STATE_MASK) != WIDGET_NORMAL)
        {
            widget_state substate(
                state.code & ~WIDGET_PRIMARY_STATE_MASK_CODE);
            path = add_substyle_to_path(&storage->nodes[3], path,
                find_substyle(search_path,
                    substyle_name + widget_state_string(substate)));
        }
    }

    // Add the original state itself.
    path = add_substyle_to_path(&storage->nodes[4], path,
        find_substyle(search_path,
            substyle_name + widget_state_string(state)));

    return path;
}

// WHOLE TREE I/O

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

static utf8_ptr skip_space(utf8_string const& text, int& line_count)
{
    utf8_ptr p = text.begin;
    while (p < text.end)
    {
        utf8_ptr q = p;
        unicode_char_t c = next_utf8_char(&p, text.end);
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

static utf8_ptr find_end_of_fallback_path(utf8_string const& text)
{
    utf8_ptr p = text.begin;
    while (p < text.end)
    {
        utf8_ptr q = p;
        unicode_char_t c = next_utf8_char(&p, text.end);
        if (is_space(c) || c == ',' || c == '{')
            return q;
    }
    return text.end;
}

static std::list<string>
parse_fallbacks(char const* label, utf8_string const& text,
    utf8_ptr& p, int& line_number)
{
    std::list<string> fallbacks;
    while (1)
    {
        p = skip_space(utf8_string(p, text.end), line_number);
        utf8_ptr subpath_start = p;
        p = find_end_of_fallback_path(utf8_string(p, text.end));
        fallbacks.push_back(string(subpath_start, p - subpath_start));
        p = skip_space(utf8_string(p, text.end), line_number);
        utf8_ptr q = p;
        unicode_char_t c = next_utf8_char(&p, text.end);
        if (c == '{')
        {
            p = q;
            break;
        }
        else if (c == ',')
        {
            continue;
        }
        else
        {
            throw parse_error(string(label) + ":" +
                to_string(line_number) + ": syntax error");
        }
    }
    return fallbacks;
}

static property_map
parse_style_properties(char const* label, utf8_string const& text,
    utf8_ptr& p, int& line_number)
{
    property_map properties;
    while (1)
    {
        p = skip_space(utf8_string(p, text.end), line_number);

        // Check for a closing brace.
        SkUnichar c = peek(utf8_string(p, text.end));
        if (c == '}')
        {
            next_utf8_char(&p, text.end);
            break;
        }

        // Parse the name.
        utf8_ptr name_start = p, name_end;
        bool name_ended = false;
        while (1)
        {
            utf8_ptr q = p;
            unicode_char_t c = next_utf8_char(&p, text.end);
            if (c == ':')
            {
                name_end = q;
                break;
            }
            if (is_space(c))
            {
                throw parse_error(string(label) + ":" +
                    to_string(line_number) + ": syntax error");
            }
        }
        string name(name_start, name_end - name_start);

        p = skip_space(utf8_string(p, text.end), line_number);

        // Parse the value.
        utf8_ptr value_start = p, value_end;
        while (1)
        {
            utf8_ptr q = p;
            unicode_char_t c = next_utf8_char(&p, text.end);
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
                value_end = q;
                break;
            }
        }
        string value(value_start, value_end - value_start);

        properties[name] = value;
    }
    return properties;
}

static void append_span(string& dst, utf8_ptr begin, utf8_ptr end)
{
    while (begin != end)
    {
        dst.push_back(*begin);
        ++begin;
    }
}

static string strip_comments(utf8_string const& text)
{
    string code;
    code.reserve(text.end - text.begin);
    utf8_ptr p = text.begin;
    int comment_depth = 0;
    SkUnichar last_char = 0;
    utf8_ptr span_start = text.begin;
    while (p != text.end)
    {
        utf8_ptr q = p;
        unicode_char_t c = next_utf8_char(&p, text.end);
        switch (c)
        {
         case '*':
            if (last_char == '/')
            {
                if (comment_depth == 0)
                    append_span(code, span_start, q - 1);
                ++comment_depth;
                last_char = 0;
                continue;
            }
            break;
         case '/':
            if (last_char == '*')
            {
                if (comment_depth > 0)
                {
                    --comment_depth;
                    span_start = p;
                    last_char = 0;
                    continue;
                }
            }
            break;
        }
        last_char = c;
        if (comment_depth > 0 && is_line_terminator(c))
        {
            p = skip_line_terminator(utf8_string(q, text.end));
            code.push_back('\n');
        }
    }
    append_span(code, span_start, text.end);
    return code;
}

style_tree_ptr
parse_style_description(char const* label, utf8_string const& text)
{
    style_tree_ptr tree;
    tree.reset(new style_tree);
    int line_number = 1;
    string stripped = strip_comments(text);
    utf8_string stripped_text = as_utf8_string(stripped);
    utf8_ptr p = stripped_text.begin;
    while (1)
    {
        p = skip_space(utf8_string(p, stripped_text.end), line_number);
        if (p == stripped_text.end)
            break;

        // Parse the substyle name.
        utf8_ptr next_space =
            find_next_space(utf8_string(p, stripped_text.end));
        string subpath(p, next_space - p);
        p = next_space;
        p = skip_space(utf8_string(p, stripped_text.end), line_number);

        flattened_style_node node;

        // Check for fallbacks.
        unicode_char_t c = next_utf8_char(&p, text.end);
        if (c == ':')
        {
            node.fallbacks =
                parse_fallbacks(label, stripped_text, p, line_number);
            c = next_utf8_char(&p, text.end);
        }

        // Check for the opening brace of the property map.
        if (c != '{')
        {
            throw parse_error(string(label) + ":" + to_string(line_number) +
                ": syntax error");
        }

        // Parse the property map.
        node.properties =
            parse_style_properties(label, stripped_text, p, line_number);

        // Add the substyle to the tree.
        set_style(*tree, subpath, node);
    }
    return tree;
}

style_tree_ptr parse_style_file(char const* path)
{
    FILE* f = fopen(path, "rb");
    if (!f)
        throw open_file_error("unable to open file: " + string(path));
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
    ++*index_counter;
    fprintf(f, "    alia::style_tree node_%i;\n", index);
    for (property_map::const_iterator
        i = node.properties.begin(); i != node.properties.end(); ++i)
    {
        fprintf(f, "    node_%i.properties[\"%s\"] = \"%s\";\n",
            index, i->first.c_str(), i->second.c_str());
    }
    for (std::map<string,style_tree_ptr>::const_iterator
        i = node.substyles.begin(); i != node.substyles.end(); ++i)
    {
        int substyle_index =
            write_cpp_style_node(f, *i->second, index_counter);
        fprintf(f, "    node_%i.substyles[\"%s\"] = node_%i;\n",
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
    fprintf(f, "    return node_0;\n");
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

string read_string(line_parser& p)
{
    skip_space(p);
    string s;
    while (!is_eol(p))
    {
        char c = peek(p);
        if (std::isspace(c))
            break;
        s.push_back(c);
        advance(p);
    }
    return s;
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

rgba8
get_color_property(style_search_path const* path, char const* property_name)
{
    return get_property(path, property_name, INHERITED_PROPERTY, rgba8(black));
}

rgba8
get_color_property(dataless_ui_context& ctx, char const* property_name)
{
    return get_property(ctx.style.path, property_name, INHERITED_PROPERTY,
        rgba8(black));
}

// layout properties

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

void parse(line_parser& p, absolute_length* spec)
{
    skip_space(p);
    parse(p, &spec->length);
    skip_space(p);
    parse(p, &spec->units);
}

void parse(line_parser& p, absolute_size* spec)
{
    parse(p, &(*spec)[0]);
    if (!is_empty(p))
        parse(p, &(*spec)[1]);
    else
        (*spec)[1] = (*spec)[0];
}

void parse(line_parser& p, relative_length* spec)
{
    skip_space(p);
    parse(p, &spec->length);
    skip_space(p);
    if (peek(p) == '%')
    {
        spec->is_relative = true;
        spec->length /= 100;
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

void parse(line_parser& p, relative_size* spec)
{
    parse(p, &(*spec)[0]);
    if (!is_empty(p))
        parse(p, &(*spec)[1]);
    else
        (*spec)[1] = (*spec)[0];
}

template<class Side>
void fill_in_missing_sides(Side* sides, int n_sides)
{
    if (n_sides < 2)
        sides[1] = sides[0];
    if (n_sides < 3)
        sides[2] = sides[0];
    if (n_sides < 4)
        sides[3] = sides[1];
}

void parse(line_parser& p, box_border_width<absolute_length>* border)
{
    absolute_length sides[4];
    int n_sides = 0;
    while (1)
    {
        skip_space(p);
        if (is_eol(p))
            break;
        if (n_sides >= 4)
            throw_unexpected_char(p);
        parse(p, &sides[n_sides++]);
    }
    if (n_sides == 0)
        throw parse_error("empty border width list");
    fill_in_missing_sides(sides, n_sides);
    border->top = sides[0];
    border->right = sides[1];
    border->bottom = sides[2];
    border->left = sides[3];
}

void parse(line_parser& p, box_corner_sizes* spec)
{
    relative_length specs[2][4];
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
    fill_in_missing_sides(specs[0], n_specs[0]);
    if (n_specs[1] == 0)
    {
        for (int i = 0; i != 4; ++i)
            specs[1][i] = specs[0][i];
    }
    else
        fill_in_missing_sides(specs[1], n_specs[1]);
    for (int i = 0; i != 4; ++i)
        spec->corners[i] = make_vector(specs[0][i], specs[1][i]);
}

resolved_box_corner_sizes
resolve_box_corner_sizes(layout_traversal& traversal,
    box_corner_sizes const& spec, vector<2,float> const& full_size)
{
    resolved_box_corner_sizes sizes;
    for (int i = 0; i != 4; ++i)
    {
        sizes.corners[i] =
            resolve_relative_size(traversal, spec.corners[i], full_size);
    }
    return sizes;
}

void parse(line_parser& p, side_selection* spec)
{
    side_selection sides = NO_FLAGS;
    while (1)
    {
        string s = read_string(p);
        if (s.empty())
            break;
        if (s == "left")
            sides |= LEFT_SIDE;
        if (s == "right")
            sides |= RIGHT_SIDE;
        if (s == "top")
            sides |= TOP_SIDE;
        if (s == "bottom")
            sides |= BOTTOM_SIDE;
    }
    *spec = sides;
}

box_border_width<absolute_length>
get_padding_property(
    style_search_path const* path, absolute_length const& default_width)
{
    box_border_width<absolute_length> unified =
        get_property(path, "padding", UNINHERITED_PROPERTY,
            box_border_width<absolute_length>(default_width));
    return box_border_width<absolute_length>(
        get_property(path, "padding-top", UNINHERITED_PROPERTY,
            unified.top),
        get_property(path, "padding-right", UNINHERITED_PROPERTY,
            unified.right),
        get_property(path, "padding-bottom", UNINHERITED_PROPERTY,
            unified.bottom),
        get_property(path, "padding-left", UNINHERITED_PROPERTY,
            unified.left));
}

box_border_width<absolute_length>
get_margin_property(
    style_search_path const* path, absolute_length const& default_width)
{
    box_border_width<absolute_length> unified =
        get_property(path, "margin", UNINHERITED_PROPERTY,
            box_border_width<absolute_length>(default_width));
    return box_border_width<absolute_length>(
        get_property(path, "margin-top", UNINHERITED_PROPERTY,
            unified.top),
        get_property(path, "margin-right", UNINHERITED_PROPERTY,
            unified.right),
        get_property(path, "margin-bottom", UNINHERITED_PROPERTY,
            unified.bottom),
        get_property(path, "margin-left", UNINHERITED_PROPERTY,
            unified.left));
}

box_border_width<absolute_length>
get_border_width_property(
    style_search_path const* path, absolute_length const& default_width)
{
    box_border_width<absolute_length> unified =
        get_property(path, "border-width", UNINHERITED_PROPERTY,
            box_border_width<absolute_length>(default_width));
    return box_border_width<absolute_length>(
        get_property(path, "border-top-width", UNINHERITED_PROPERTY,
            unified.top),
        get_property(path, "border-right-width", UNINHERITED_PROPERTY,
            unified.right),
        get_property(path, "border-bottom-width", UNINHERITED_PROPERTY,
            unified.bottom),
        get_property(path, "border-left-width", UNINHERITED_PROPERTY,
            unified.left));
}

box_corner_sizes
get_border_radius_property(
    style_search_path const* path, relative_length const& default_radius)
{
    box_corner_sizes unified =
        get_property(path, "border-radius", UNINHERITED_PROPERTY,
            box_corner_sizes(
                make_vector(default_radius, default_radius)));
    return box_corner_sizes(
        get_property(path, "border-top-left-radius", UNINHERITED_PROPERTY,
            unified.corners[0]),
        get_property(path, "border-top-right-radius", UNINHERITED_PROPERTY,
            unified.corners[1]),
        get_property(path, "border-bottom-right-radius", UNINHERITED_PROPERTY,
            unified.corners[2]),
        get_property(path, "border-bottom-left-radius", UNINHERITED_PROPERTY,
            unified.corners[3]));
}

// higher-level retrieval

font get_font_properties(ui_system const& ui, style_search_path const* path)
{
    return font(
        get_property(path, "font-family", INHERITED_PROPERTY, string("arial")),
        get_property(path, "font-size", INHERITED_PROPERTY, 13.f) *
            ui.style.magnification,
        (get_property(path, "font-bold", INHERITED_PROPERTY, false) ?
            BOLD : NO_FLAGS) |
        (get_property(path, "font-italic", INHERITED_PROPERTY, false) ?
            ITALIC : NO_FLAGS) |
        (get_property(path, "font-underline", INHERITED_PROPERTY, false) ?
            UNDERLINE : NO_FLAGS) |
        (get_property(path, "font-strikethrough", INHERITED_PROPERTY, false) ?
            STRIKETHROUGH : NO_FLAGS));
}

void read_primary_style_properties(
    ui_system const& ui,
    primary_style_properties* props,
    style_search_path const* path)
{
    props->text_color = get_color_property(path, "color");
    props->background_color = get_color_property(path, "background");
    props->font = get_font_properties(ui, path);
}

// default_padding_spec is simply an absolute_size, but it parses height before
// width to be consistent with normal CSS-style padding specifications.
struct default_padding_spec
{
    absolute_size padding;
    default_padding_spec() {}
    default_padding_spec(absolute_size const& padding) : padding(padding) {}
};
void parse(line_parser& p, default_padding_spec* spec)
{
    parse(p, &spec->padding[1]);
    if (!is_empty(p))
        parse(p, &spec->padding[0]);
    else
        spec->padding[0] = spec->padding[1];
}

void read_layout_style_info(
    dataless_ui_context& ctx, layout_style_info* style_info,
    font const& font, style_search_path const* path)
{
    style_info->magnification = ctx.system->style.magnification;

    style_info->font_size = font.size;

    // Skia supposedly supplies all the necessary font metrics, but they're
    // not always valid.
    //SkPaint paint;
    //set_skia_font_info(paint, font);
    //SkPaint::FontMetrics metrics;
    //SkScalar line_spacing = paint.getFontMetrics(&metrics);
    //style_info->character_size = make_vector(
    //    SkScalarToFloat(metrics.fAvgCharWidth),
    //    SkScalarToFloat(line_spacing));
    //style_info->x_height = SkScalarToFloat(metrics.fXHeight);

    // ... so do some approximations instead.
    style_info->character_size = make_vector(font.size * 0.6f, font.size);
    style_info->x_height = font.size * 0.5f;

    // The padding size may be specified in terms of the above properties,
    // so now that those are set, we can evaluated padding size using the
    // style_info structure as a reference.
    if (get_property(path, "disable-padding", UNINHERITED_PROPERTY, false))
    {
        style_info->padding_size = make_layout_vector(0, 0);
    }
    else
    {
        default_padding_spec default_padding =
            get_property(path, "default-padding", INHERITED_PROPERTY,
                default_padding_spec(make_vector(
                    absolute_length(0.2f, EM),
                    absolute_length(0.2f, EM))));
        style_info->padding_size = as_layout_size(
            resolve_absolute_size(ctx.layout->ppi, *style_info,
                default_padding.padding));
    }
}

void update_substyle_data(
    dataless_ui_context& ctx, substyle_data& data,
    style_search_path const* path,
    string const& substyle_name, widget_state state,
    add_substyle_flag_set flags)
{
    inc_version(data.identity);

    data.state.path =
        add_substyle_to_path(&data.path_storage, path, path, substyle_name,
            state, flags);

    read_primary_style_properties(
        *ctx.system, &data.properties, data.state.path);
    data.state.properties = &data.properties;

    data.state.theme = ctx.style.theme;

    data.state.id = &data.id;

    read_layout_style_info(ctx, &data.style_info, data.properties.font,
        data.state.path);

    data.id = get_id(data.identity);
}

keyed_data<substyle_data>*
get_substyle_data(
    ui_context& ctx, accessor<string> const& substyle_name, widget_state state,
    scoped_substyle_flag_set flags)
{
    keyed_data<substyle_data>* data;
    if (get_cached_data(ctx, &data) || is_refresh_pass(ctx))
    {
        refresh_keyed_data(*data, combine_ids(ref(ctx.style.id),
            combine_ids(ref(&substyle_name.id()), make_id(state))));
    }
    if (!is_valid(*data))
    {
        update_substyle_data(ctx, data->value, ctx.style.path,
            get(substyle_name), state,
            (flags & SCOPED_SUBSTYLE_NO_PATH_SEPARATOR) ?
                ADD_SUBSTYLE_NO_PATH_SEPARATOR : NO_FLAGS);
        mark_valid(*data);
    }
    return data;
}

}
