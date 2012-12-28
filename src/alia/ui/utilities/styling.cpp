#include <alia/ui/utilities/styling.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

string const* get_style_property(
    style_tree const* tree, char const* property_name)
{
    if (tree)
    {
        property_map::const_iterator i = tree->properties.find(property_name);
        if (i != tree->properties.end())
            return &i->second;
    }
    return 0;
}

string const* get_style_property(
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

void read_primary_style_properties(
    ui_system const& ui,
    primary_style_properties* props,
    style_search_path const* path)
{
    props->text_color =
        get_color_property(path, "text_color",
            rgba8(0x00, 0x00, 0x00, 0xff));
    props->background_color =
        get_color_property(path, "background_color",
            rgba8(0xff, 0xff, 0xff, 0xff));
    props->font = get_font_properties(ui, path);
}

void read_layout_style_info(layout_style_info* style_info,
    font const& font, style_search_path const* path)
{
    style_info->is_padded = get_boolean_property(path, "padded", false);
    layout_scalar padding_size =
        as_layout_size(get_float_property(path, "padding_size", 0.2f) *
            font.size);
    style_info->padding_size = make_vector(padding_size, padding_size);

    style_info->font_size = font.size;

    SkPaint paint;
    set_skia_font_info(paint, font);

    SkPaint::FontMetrics metrics;
    SkScalar line_spacing = paint.getFontMetrics(&metrics);

    style_info->character_size = make_vector(
        metrics.fAvgCharWidth > 0 ? SkScalarToFloat(metrics.fAvgCharWidth) :
            SkScalarToFloat(line_spacing) * 0.6f,
        SkScalarToFloat(line_spacing));

    style_info->x_height =
        metrics.fXHeight > 0 ? SkScalarToFloat(metrics.fXHeight) :
            SkScalarToFloat(line_spacing) * 0.5f;
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

style_tree const* find_substyle(style_search_path const* path,
    string const& substyle_name)
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

style_tree const* find_substyle(style_search_path const* path,
    string const& substyle_name, widget_state state)
{
    style_tree const* substyle;
    substyle = find_substyle(path, substyle_name + widget_state_string(state));
    if (substyle)
        return substyle;
    if (state & WIDGET_FOCUSED)
    {
        widget_state substate(state.code & ~WIDGET_FOCUSED_CODE);
        substyle = find_substyle(path,
            substyle_name + widget_state_string(substate));
        if (substyle)
            return substyle;
    }
    if ((state & WIDGET_PRIMARY_STATE_MASK) != WIDGET_NORMAL)
    {
        widget_state substate(
            state.code & ~WIDGET_PRIMARY_STATE_MASK_CODE);
        substyle = find_substyle(path,
            substyle_name + widget_state_string(substate));
        if (substyle)
            return substyle;
    }
    if ((state & WIDGET_FOCUSED) &&
        (state & WIDGET_PRIMARY_STATE_MASK) != WIDGET_NORMAL)
    {
        substyle = find_substyle(path, substyle_name);
        if (substyle)
            return substyle;
    }
    return 0;
}

static rgba8 parse_color_value(char const* value)
{
    if (*value != '#')
        throw parse_error("invalid hex color constant");
    ++value;

    uint8_t digits[8];
    bool has_alpha;
    for (int i = 0; ; ++i)
    {
        char c = value[i];
        if (c == '\0')
        {
            switch (i)
            {
             case 6:
                has_alpha = false;
                break;
             case 8:
                has_alpha = true;
                break;
             default:
                throw parse_error("invalid hex color constant");
            }
            break;
        }
        else if (i >= 8)
            throw parse_error("invalid hex color constant");
        else if (c >= '0' && c <= '9')
            digits[i] = uint8_t(c - '0');
        else if (c >= 'a' && c <= 'f')
            digits[i] = 10 + uint8_t(c - 'a');
        else if (c >= 'A' && c <= 'F')
            digits[i] = 10 + uint8_t(c - 'A');
        else
            throw parse_error("invalid hex color constant");
    }

    alia::rgba8 color;
    color.r = (digits[0] << 4) + digits[1];
    color.g = (digits[2] << 4) + digits[3];
    color.b = (digits[4] << 4) + digits[5];
    color.a = has_alpha ? ((digits[6] << 4) + digits[7]) : 0xff;
    return color;
}

rgba8 get_color_property(style_tree const* tree,
    char const* property_name, rgba8 default_value)
{
    string const* value = get_style_property(tree, property_name);
    return value ? parse_color_value(value->c_str()) : default_value;
}
rgba8 get_color_property(style_search_path const* path,
    char const* property_name, rgba8 default_value)
{
    string const* value = get_style_property(path, property_name);
    return value ? parse_color_value(value->c_str()) : default_value;
}

int get_integer_property(style_tree const* tree,
    char const* property_name, int default_value)
{
    string const* value = get_style_property(tree, property_name);
    string message;
    int n;
    if (value && from_string(&n, *value, &message))
        return n;
    return default_value;
}
int get_integer_property(style_search_path const* path,
    char const* property_name, int default_value)
{
    string const* value = get_style_property(path, property_name);
    string message;
    int n;
    if (value && from_string(&n, *value, &message))
        return n;
    return default_value;
}

float get_float_property(style_tree const* tree,
    char const* property_name, float default_value)
{
    string const* value = get_style_property(tree, property_name);
    string message;
    float n;
    if (value && from_string(&n, *value, &message))
        return n;
    return default_value;
}
float get_float_property(style_search_path const* path,
    char const* property_name, float default_value)
{
    string const* value = get_style_property(path, property_name);
    string message;
    float n;
    if (value && from_string(&n, *value, &message))
        return n;
    return default_value;
}

string get_string_property(style_tree const* tree,
    char const* property_name, string const& default_value)
{
    string const* value = get_style_property(tree, property_name);
    return value ? *value : default_value;
}
string get_string_property(style_search_path const* path,
    char const* property_name, string const& default_value)
{
    string const* value = get_style_property(path, property_name);
    return value ? *value : default_value;
}

bool get_boolean_property(style_tree const* tree,
    char const* property_name, bool default_value)
{
    string const* value = get_style_property(tree, property_name);
    return value ? *value == "true" : default_value;
}
bool get_boolean_property(style_search_path const* path,
    char const* property_name, bool default_value)
{
    string const* value = get_style_property(path, property_name);
    return value ? *value == "true" : default_value;
}

font get_font_properties(ui_system const& ui, style_search_path const* path)
{
    return font(
        get_string_property(path, "font_name", "arial"),
        get_float_property(path, "font_size", 13) *
            ui.style->text_magnification,
        (get_boolean_property(path, "font_bold", false) ?
            BOLD : NO_FLAGS) |
        (get_boolean_property(path, "font_italic", false) ?
            ITALIC : NO_FLAGS) |
        (get_boolean_property(path, "font_underline", false) ?
            UNDERLINE : NO_FLAGS) |
        (get_boolean_property(path, "font_strikethrough", false) ?
            STRIKETHROUGH : NO_FLAGS));
}

struct initial_styling_data
{
    owned_id id;
    primary_style_properties props;
    layout_style_info info;
    style_search_path path;
};

void setup_initial_styling(ui_context& ctx)
{
    initial_styling_data* data;
    get_data(ctx, &data);

    if (!data->id.matches(get_id(ctx.system->style->id)))
    {
        data->path.rest = 0;
        data->path.tree = &ctx.system->style->styles;

        read_primary_style_properties(
            *ctx.system, &data->props, &data->path);

        data->id.store(get_id(ctx.system->style->id));

        read_layout_style_info(&data->info, data->props.font, &data->path);
        ctx.layout.style_info = &data->info;
    }
    ctx.style.path = &data->path;
    ctx.style.properties = &data->props;
    ctx.style.id = &data->id.get();
    ctx.style.theme = &ctx.system->style->theme;
}

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

}
