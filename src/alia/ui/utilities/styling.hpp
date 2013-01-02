#ifndef ALIA_UI_UTILITIES_STYLING_HPP
#define ALIA_UI_UTILITIES_STYLING_HPP

#include <alia/ui/internals.hpp>

// This file provides various utilities for working with the UI styling system.

namespace alia {

void set_style(style_tree& tree, string const& subpath,
    property_map const& properties);

style_tree unflatten_style_tree(flattened_style_tree const& flattened);

struct parse_error : exception
{
    parse_error(string const& message)
      : exception(message)
    {}
    ~parse_error() throw() {}
};

style_tree parse_style_description(char const* label, utf8_string const& text);

style_tree parse_style_file(char const* path);

// Set up the initial styling state for a context pass.
void setup_initial_styling(ui_context& ctx);

// Look up the value of a property in the style tree.
string const* get_style_property(
    style_tree const* tree, char const* property_name);
// Look up the value of a property in the style search path.
string const* get_style_property(
    style_search_path const* path,
    char const* property_name);

// Find a substyle within the given style search path.
style_tree const* find_substyle(style_search_path const* path,
    string const& substyle_name);

// Find a substyle with an associated widget state within the given style
// search path.
style_tree const* find_substyle(style_search_path const* path,
    string const& substyle_name, widget_state state);

// The following are all utility functions for retrieving property values of
// various types from from style trees, style search paths, or the search path
// associated with a particular UI context.

rgba8 get_color_property(
    style_tree const* tree, char const* property_name,
    rgba8 default_value = rgba8(0xff, 0xff, 0xff, 0xff));
rgba8 get_color_property(
    style_search_path const* path, char const* property_name,
    rgba8 default_value = rgba8(0xff, 0xff, 0xff, 0xff));
static inline rgba8 get_color_property(
    ui_context& ctx, char const* property_name,
    rgba8 default_value = rgba8(0xff, 0xff, 0xff, 0xff))
{ return get_color_property(ctx.style.path, property_name, default_value); }

int get_integer_property(style_tree const* tree,
    char const* property_name, int default_value);
int get_integer_property(style_search_path const* path,
    char const* property_name, int default_value);
static inline int get_integer_property(ui_context& ctx,
    char const* property_name, int default_value)
{ return get_integer_property(ctx.style.path, property_name, default_value); }

float get_float_property(style_tree const* tree,
    char const* property_name, float default_value);
float get_float_property(style_search_path const* path,
    char const* property_name, float default_value);
static inline float get_float_property(ui_context& ctx,
    char const* property_name, float default_value)
{ return get_float_property(ctx.style.path, property_name, default_value); }

string get_string_property(style_tree const* tree,
    char const* property_name, string const& default);
string get_string_property(style_search_path const* path,
    char const* property_name, string const& default);
static inline string get_string_property(ui_context& ctx,
    char const* property_name, string const& default_value)
{ return get_string_property(ctx.style.path, property_name, default_value); }

bool get_boolean_property(style_tree const* tree,
    char const* property_name, bool default_value);
bool get_boolean_property(style_search_path const* path,
    char const* property_name, bool default_value);
static inline bool get_boolean_property(ui_context& ctx,
    char const* property_name, bool default_value)
{ return get_boolean_property(ctx.style.path, property_name, default_value); }

font get_font_properties(ui_system const& ui, style_search_path const* path);

void read_primary_style_properties(
    ui_system const& ui,
    primary_style_properties* props,
    style_search_path const* path);

void read_layout_style_info(layout_style_info* style_info,
    font const& font, style_search_path const* path);

}

#endif
