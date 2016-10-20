#ifndef ALIA_UI_UTILITIES_STYLING_HPP
#define ALIA_UI_UTILITIES_STYLING_HPP

#include <alia/ui/internals.hpp>

// This file provides various utilities for working with the UI styling system.

namespace alia {

// STYLE TREE MANIPULATION

// A flattened style tree is one that simply stores subpaths and their
// associated style nodes. Subpaths are separated by '/'.

struct flattened_style_node
{
    std::list<string> fallbacks;
    property_map properties;
};

typedef std::map<string,flattened_style_node> flattened_style_tree;

// Update the given tree so that the given subpath is set to the given flat
// style node.
void set_style(style_tree& tree, string const& subpath,
    flattened_style_node const& flattened);

// Convert a flattened style tree to a normal one.
style_tree_ptr unflatten_style_tree(flattened_style_tree const& flattened);

// STYLE PROPERTY SEARCHING

// All searching is done through search paths. Search paths are linked lists
// of pointers to style trees. A null pointer within the list acts as a
// separator between atomic blocks of trees. When searching for a property
// that is NOT inherited, searching stops when it encounters a separator.

ALIA_DEFINE_FLAG_TYPE(style_search)
ALIA_DEFINE_FLAG(style_search, 0x00, UNINHERITED_PROPERTY)
ALIA_DEFINE_FLAG(style_search, 0x01, INHERITED_PROPERTY)

// Look up the value of a property using the given search path.
string const*
get_style_property(
    style_search_path const* path, char const* property_name,
    style_search_flag_set flags);

// Find a substyle within the search path and construct a new path consisting
// of the substyle followed by the 'rest' path.
// Since the return value is a pointer to the new path, the caller must supply
// storage for it.
struct style_path_storage
{
    style_search_path nodes[2];
};
ALIA_DEFINE_FLAG_TYPE(add_substyle)
ALIA_DEFINE_FLAG(add_substyle, 0x0000040, ADD_SUBSTYLE_NO_PATH_SEPARATOR)
ALIA_DEFINE_FLAG(add_substyle, 0x0000040, ADD_SUBSTYLE_IFF_EXISTS)
style_search_path const*
add_substyle_to_path(
    style_path_storage* storage,
    style_search_path const* search_path,
    style_search_path const* rest,
    string const& substyle_name,
    add_substyle_flag_set flags = NO_FLAGS);

// Same as above, but includes an associated widget state.
// This will add the substyle with and without associated state to the path,
// so the style without the associated state acts as a fallback.
// Depending on the widget state, it may also add other intermediate styles
// using simpler versions of the state.
// All substyles are added as a single atomic block.
struct stateful_style_path_storage
{
    style_search_path nodes[5];
};
style_search_path const*
add_substyle_to_path(
    stateful_style_path_storage* storage,
    style_search_path const* search_path,
    style_search_path const* rest,
    string const& substyle_name,
    widget_state state,
    add_substyle_flag_set flags = NO_FLAGS);

// WHOLE TREE I/O - I/O utilities that work with whole style trees.

style_tree_ptr
parse_style_description(char const* label, utf8_string const& text);

style_tree_ptr
parse_style_file(char const* path);

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

struct open_file_error : exception
{
    open_file_error(string const& message)
      : exception(message)
    {}
    ~open_file_error() throw() {}
};

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
    style_search_flag_set flags, Value default_value)
{
    return parse_property(
        get_style_property(path, property_name, flags), default_value);
}

template<class Value>
Value
get_property(
    dataless_ui_context& ctx, char const* property_name,
    style_search_flag_set flags, Value default_value)
{ return get_property(ctx.style.path, property_name, flags, default_value); }

template<class Value>
Value
get_cached_property(
    ui_context& ctx, style_search_path const* path, char const* property_name,
    style_search_flag_set flags, Value default_value)
{
    keyed_data<Value>* data;
    get_cached_data(ctx, &data);
    refresh_keyed_data(*data, *ctx.style.id);
    if (!is_valid(*data))
        set(*data, get_property(path, property_name, flags, default_value));
    return get(*data);
}

template<class Value>
Value
get_cached_property(
    ui_context& ctx, char const* property_name, style_search_flag_set flags,
    Value default_value)
{
    return get_cached_property(ctx, ctx.style.path, property_name, flags,
        default_value);
}

// color properties

void parse(line_parser& p, rgba8* color);

rgba8
get_color_property(style_search_path const* path, char const* property_name);

rgba8
get_color_property(dataless_ui_context& ctx, char const* property_name);

// layout properties

void parse(line_parser& p, layout_units* units);

void parse(line_parser& p, absolute_length* spec);
void parse(line_parser& p, absolute_size* spec);

void parse(line_parser& p, relative_length* spec);
void parse(line_parser& p, relative_size* spec);

// 2D size specification for each of the four corners of a rectangle
struct box_corner_sizes
{
    relative_size corners[4];

    box_corner_sizes(
        relative_size const& top_left,
        relative_size const& top_right,
        relative_size const& bottom_right,
        relative_size const& bottom_left)
    {
        corners[0] = top_left;
        corners[1] = top_right;
        corners[2] = bottom_right;
        corners[3] = bottom_left;
    }
    explicit box_corner_sizes(relative_size const& corner)
    {
        for (int i = 0; i != 4; ++i)
            corners[i] = corner;
    }
    box_corner_sizes() {}
};

struct resolved_box_corner_sizes
{
    vector<2,float> corners[4];
};

resolved_box_corner_sizes
resolve_box_corner_sizes(layout_traversal& traversal,
    box_corner_sizes const& spec, vector<2,float> const& full_size);

void parse(line_parser& p, box_corner_sizes* spec);

void parse(line_parser& p, box_border_width<absolute_length>* spec);

box_border_width<absolute_length>
get_padding_property(
    style_search_path const* path,
    absolute_length const& default_width = absolute_length(0, PIXELS));

box_border_width<absolute_length>
get_margin_property(
    style_search_path const* path,
    absolute_length const& default_width = absolute_length(0, PIXELS));

box_border_width<absolute_length>
get_border_width_property(
    style_search_path const* path,
    absolute_length const& default_width = absolute_length(0, PIXELS));

box_corner_sizes
get_border_radius_property(
    style_search_path const* path,
    relative_length const& default_radius = relative_length(0, PIXELS));

// specification that selects sides of a rectangle
struct side_selection_flag_tag {};
typedef flag_set<side_selection_flag_tag> side_selection;
ALIA_DEFINE_FLAG(side_selection, 0x0, NO_SIDES)
ALIA_DEFINE_FLAG(side_selection, 0x1, LEFT_SIDE)
ALIA_DEFINE_FLAG(side_selection, 0x2, RIGHT_SIDE)
ALIA_DEFINE_FLAG(side_selection, 0x4, TOP_SIDE)
ALIA_DEFINE_FLAG(side_selection, 0x8, BOTTOM_SIDE)

void parse(line_parser& p, side_selection* spec);

// HIGHER-LEVEL STYLE AND PROPERTY RETRIEVAL UTILITIES

// This is used for caching compound style info structures.
//
// To use it, define the style info structure and provide a function of the
// following form.
//
// void read_style_info(dataless_ui_context& ctx, Info* info,
//    style_search_path const* path);
//
// This should read the necessary style info from the UI context and style
// path and store it in *info.
//
// You can then call get_cached_style_info(ctx, &info, style) each pass.
// This will take care of caching and calling read_style_info when needed.
// On return, *info always points to the latest style info.
//
template<class Info>
void
get_cached_style_info(ui_context& ctx, Info const** info,
    alia::accessor<string> const& style)
{
    keyed_data<Info>* cached_info;
    if (get_cached_data(ctx, &cached_info) || is_refresh_pass(ctx))
    {
        refresh_keyed_data(*cached_info,
            combine_ids(ref(ctx.style.id), ref(&style.id())));
        if (!is_valid(*cached_info))
        {
            style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, ctx.style.path,
                    get(style));
            Info info;
            read_style_info(ctx, &info, path);
            set(*cached_info, info);
        }
    }
    assert(is_valid(*cached_info));
    *info = &get(*cached_info);
}

// Same as above, but for stateful components.
template<class Info>
void
get_cached_style_info(ui_context& ctx, Info const** info,
    alia::accessor<string> const& style, widget_state state)
{
    keyed_data<Info>* cached_info;
    if (get_cached_data(ctx, &cached_info) || is_refresh_pass(ctx))
    {
        refresh_keyed_data(*cached_info,
            combine_ids(ref(ctx.style.id),
                combine_ids(ref(&style.id()), make_id(state))));
        if (!is_valid(*cached_info))
        {
            stateful_style_path_storage storage;
            style_search_path const* path =
                add_substyle_to_path(&storage, ctx.style.path, ctx.style.path,
                    get(style), state);
            Info info;
            read_style_info(ctx, &info, path);
            set(*cached_info, info);
        }
    }
    assert(is_valid(*cached_info));
    *info = &get(*cached_info);
}

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
void read_layout_style_info(
    dataless_ui_context& ctx, layout_style_info* style_info,
    font const& font, style_search_path const* path);

struct substyle_data : noncopyable
{
    stateful_style_path_storage path_storage;
    primary_style_properties properties;
    style_state state;
    layout_style_info style_info;
    value_id_by_reference<local_id> id;
    local_identity identity;
};

void update_substyle_data(
    dataless_ui_context& ctx, substyle_data& data,
    style_search_path const* path,
    string const& substyle_name, widget_state state,
    add_substyle_flag_set flags = NO_FLAGS);

keyed_data<substyle_data>*
get_substyle_data(
    ui_context& ctx, accessor<string> const& substyle_name,
    widget_state state = WIDGET_NORMAL,
    scoped_substyle_flag_set flags = NO_FLAGS);

}

#endif
