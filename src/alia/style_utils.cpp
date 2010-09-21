#include <alia/style_utils.hpp>

namespace alia {

struct scoped_substyle_data
{
    style_node const* node;
    std::string name;
    primary_style_properties props;
};

void scoped_substyle::begin(context& ctx, char const* name,
    widget_state state)
{
    ctx_ = &ctx;

    old_style_ = ctx.pass_state.active_style;
    old_props_ = ctx.pass_state.style;

    scoped_substyle_data* data;
    if (get_cached_style_data(ctx, &data, state) || data->name != name)
    {
        data->node = get_substyle(old_style_, name, state);
        data->name = name;
        get_style_properties(ctx, data->node, data->props);
    }

    ctx.pass_state.active_style = data->node;
    ctx.pass_state.style = &data->props;

    active_ = true;
}

void scoped_substyle::begin(context& ctx, char const* name,
    const_style_name_type _)
{
    ctx_ = &ctx;

    old_style_ = ctx.pass_state.active_style;
    old_props_ = ctx.pass_state.style;

    scoped_substyle_data* data;
    if (get_cached_style_data(ctx, &data))
    {
        data->node = get_substyle(old_style_, name);
        get_style_properties(ctx, data->node, data->props);
    }

    ctx.pass_state.active_style = data->node;
    ctx.pass_state.style = &data->props;

    active_ = true;
}

void scoped_substyle::end()
{
    if (active_)
    {
        ctx_->pass_state.active_style = old_style_;
        ctx_->pass_state.style = old_props_;
        active_ = false;
    }
}

void get_style_properties(context& ctx, style_node const* style,
    primary_style_properties& props, widget_state state)
{
    bool padded = false;
    get_property(&padded, style, "padded", state);
    props.padding_size = padded ?
        vector2i(int(2 * ctx.font_scale_factor + 0.5), 2) :
        vector2i(0, 0);

    get_font_property(&props.font, ctx, style, "font", state);

    get_color_property(&props.text_color, style, "color", state);
    get_color_property(&props.bg_color, style, "background_color", state);
    get_color_property(&props.selected_text_color, style, "selected/color",
        state);
    get_color_property(&props.selected_bg_color, style,
        "selected/background_color", state);
}

void get_font_property(font* result, context& ctx, style_node const* style,
    std::string const& subpath, widget_state state)
{
    std::string name;
    get_property(&name, style, subpath + "_name", state);
    bool is_bold = false;
    get_property(&is_bold, style, subpath + "_bold", state);
    bool is_italic = false;
    get_property(&is_italic, style, subpath + "_italic", state);
    bool is_underlined = false;
    get_property(&is_underlined, style, subpath + "_underlined", state);
    double size = 12;
    get_property(&size, style, subpath + "_size", state);
    *result = alia::font(name, float(size) * ctx.font_scale_factor,
        (is_bold ? alia::font::BOLD : 0) |
        (is_italic ? alia::font::ITALIC : 0) |
        (is_underlined ? alia::font::UNDERLINED : 0));
}

void get_color_property(rgba8* result, style_node const* style,
    std::string const& subpath, widget_state state)
{
    if (!get_property(result, style, subpath, state))
        *result = rgba8(0, 0, 0, 0);
}

void get_numeric_property(double* result, style_node const* style,
    std::string const& subpath, widget_state state)
{
    if (!get_property(result, style, subpath, state))
        *result = 0;
}

void get_numeric_property(int* result, style_node const* style,
    std::string const& subpath, widget_state state)
{
    double n;
    if (get_property(&n, style, subpath, state))
        *result = int(n + 0.5);
    else
        *result = 0;
}

}
