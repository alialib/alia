#ifndef ALIA_UI_UTILITIES_STYLING_HPP
#define ALIA_UI_UTILITIES_STYLING_HPP

#include <alia/ui/internals.hpp>

// This file provides various utilities for working with the UI styling system.

namespace alia {

// STYLE TREE MANIPULATION 

// Update the given tree so that the given subpath is set to the given property
// map. Elements within the subpath are separated by '/'.
void set_style(style_tree& tree, string const& subpath,
    property_map const& properties);

// A flattened style tree is one that simply stored subpaths and their
// associated property maps.
typedef std::map<string,property_map> flattened_style_tree;

// Convert a flattened style tree to a normal one.
style_tree unflatten_style_tree(flattened_style_tree const& flattened);

// Look up the value of a property in the style search path.
string const* get_style_property(
    style_search_path const* path,
    char const* property_name);

// Find a substyle within the search path and construct a new path consisting
// of the substyle followed by the 'rest' path.
// Since the return value is a pointer to the new path, the caller must supply
// storage for a single path node.
style_search_path const*
add_substyle_to_path(
    style_search_path* storage,
    style_search_path const* search_path,
    style_search_path const* rest,
    string const& substyle_name);

// Same as above, but includes an associated widget state.
// This will add the substyle with and without associated state to the path,
// so the style without the associated state acts as a fallback.
// Depending on the widget state, it may also add other intermediate styles
// using simpler versions of the state.
struct stateful_style_path_storage
{
    style_search_path nodes[4];
};
style_search_path const*
add_substyle_to_path(
    stateful_style_path_storage* storage,
    style_search_path const* search_path,
    style_search_path const* rest,
    string const& substyle_name,
    widget_state state);

// WHOLE TREE I/O - I/O utilities that work with whole style trees.

style_tree parse_style_description(char const* label, utf8_string const& text);

style_tree parse_style_file(char const* path);

void write_style_cpp_file(char const* path, char const* label,
    style_tree const& style);

// LINE PARSING - Individual style properties are single lines, so these
// utilities can be used to parse them.

// for parsing a simple line of ASCII text
struct line_parser
{
    // original text
    char const* text;
    size_t text_size;
    // current position within the text
    char const* p;
};

void initialize_line_parser(line_parser& p, char const* text, size_t size);
void initialize_line_parser(line_parser& p, std::string const& text);

struct parse_error : exception
{
    parse_error(string const& message)
      : exception(message)
    {}
    ~parse_error() throw() {}
};

// If an unexpected character is encountered, call this to throw a parse_error.
void throw_unexpected_char(line_parser& p);

// Peek at the next character.
static inline char peek(line_parser& p) { return *p.p; }

// Advance past the next character.
static inline void advance(line_parser& p) { ++p.p; }

// Check that the next character is as expected and skip it.
// If it's not the expected character, throw an error.
void check_char(line_parser& p, char expected);

// Is the parser at the end of the current line?
static bool is_eol(line_parser& p) { return peek(p) == '\0'; }

// Check that the parser is at the end of the line.
// If it's not, throw an error.
void check_eol(line_parser& p);

// Is the rest of the line whitespace?
bool is_empty(line_parser& p);

// Check that the rest of the line is whitespace.
// If it's not, throw an error.
void check_empty(line_parser& p);

// Skip over whitespace.
void skip_space(line_parser& p);

// Skip over whitespace and then parse a number from the parser.
void parse(line_parser& p, double* x);
void parse(line_parser& p, float* x);
void parse(line_parser& p, int* x);

// PROPERTY UTILITIES

template<class Value>
Value parse_property(string const* text, Value default_value)
{
    if (text)
    {
        line_parser p;
        initialize_line_parser(p, *text);
        Value value;
        parse(p, &value);
        check_empty(p);
        return value;
    }
    else
        return default_value;
}

static inline
string parse_property(string const* text, string const& default_value)
{
    return text ? *text : default_value;
}

static inline
bool parse_property(string const* text, bool default_value)
{
    return text ? *text == "true" : default_value;
}

template<class Value>
Value
get_property(
    style_search_path const* path, char const* property_name,
    Value default_value)
{
    return parse_property(
        get_style_property(path, property_name), default_value);
}

template<class Value>
Value
get_property(
    ui_context& ctx, char const* property_name, Value default_value)
{ return get_property(ctx.style.path, property_name, default_value); }

template<class Value>
Value
get_cached_property(
    ui_context& ctx, style_search_path const* path, char const* property_name,
    Value default_value)
{
    ALIA_GET_CACHED_DATA(keyed_data<Value>)
    refresh_keyed_data(data, *ctx.style.id);
    if (!is_valid(data))
        set(data, get_property(path, property_name, default_value));
    return get(data);
}

template<class Value>
Value
get_cached_property(
    ui_context& ctx, char const* property_name, Value default_value)
{
    return get_cached_property(ctx, ctx.style.path, property_name,
        default_value);
}

void parse(line_parser& p, rgba8* color);

// CSS-style size specifications

// Absolute size specs are used to specify the size of stand-alone widgets.

struct absolute_size_spec
{
    float size;
    layout_units units;

    absolute_size_spec(float size, layout_units units)
      : size(size), units(units)
    {}
    absolute_size_spec() {}
};

void parse(line_parser& p, layout_units* units);

void parse(line_parser& p, absolute_size_spec* spec);

float
eval_absolute_size_spec(
    ui_context& ctx, unsigned axis, absolute_size_spec const& spec);

// Relative size specs are used to sepcify the size of widget components.
// They can be either be specified in normal units or as a fraction of the
// full widget size.

struct relative_size_spec
{
    bool is_relative;
    float size;
    layout_units units; // only relevant if is_relative is false

    relative_size_spec(float size, layout_units units)
      : is_relative(false), size(size), units(units)
    {}
    relative_size_spec(float size)
      : is_relative(true), size(size)
    {}
    relative_size_spec() {}
};

void parse(line_parser& p, relative_size_spec* spec);

float
eval_relative_size_spec(
    ui_context& ctx, unsigned axis, relative_size_spec const& spec,
    float full_size);

// 2D version of absolute_size_spec

struct absolute_size_spec_2d
{
    absolute_size_spec axes[2];

    absolute_size_spec_2d(
        absolute_size_spec const& width,
        absolute_size_spec const& height)
    {
        axes[0] = width;
        axes[1] = height;
    }
    absolute_size_spec_2d() {}
};

void parse(line_parser& p, absolute_size_spec_2d* spec);

vector<2,float>
eval_absolute_size_spec(ui_context& ctx, absolute_size_spec_2d const& spec);

// 2D version of relative_size_spec

struct relative_size_spec_2d
{
    relative_size_spec axes[2];

    relative_size_spec_2d(
        relative_size_spec const& width,
        relative_size_spec const& height)
    {
        axes[0] = width;
        axes[1] = height;
    }
    relative_size_spec_2d() {}
};

void parse(line_parser& p, relative_size_spec_2d* spec);

vector<2,float>
eval_relative_size_spec(ui_context& ctx, relative_size_spec_2d const& spec,
    vector<2,float> const& full_size);

// 2D size specification for each of the four corners of a rectangle

struct four_corners_spec
{
    relative_size_spec_2d corners[4];

    explicit four_corners_spec(relative_size_spec_2d corner)
    {
        for (int i = 0; i != 4; ++i)
            corners[i] = corner;
    }
    four_corners_spec() {}
};

void parse(line_parser& p, four_corners_spec* spec);

struct four_corners_sizes
{
    vector<2,float> corners[4];
};

four_corners_sizes
eval_four_corners(ui_context& ctx, four_corners_spec const& spec,
    vector<2,float> const& full_size);

// higher-level properties

// Get the entire description of a font from the given style search path.
// This requires a reference to the associated UI system because that provides
// a global font scale factor.
font get_font_properties(ui_system const& ui, style_search_path const* path);

// Read the primary style properties from the given search path.
// (The primary style properties include the font.)
void read_primary_style_properties(
    ui_system const& ui,
    primary_style_properties* props,
    style_search_path const* path);

// Read the style info that's necessary for layout from the given path.
void read_layout_style_info(layout_style_info* style_info,
    font const& font, style_search_path const* path);

}

#endif
