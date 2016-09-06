#ifndef ALIA_UI_LIBRARY_CONTROLS_HPP
#define ALIA_UI_LIBRARY_CONTROLS_HPP

#include <alia/ui/utilities.hpp>

// This file declares utilities for implementing simple controls.

namespace alia {

template<class Value>
struct simple_control_renderer : dispatch_interface
{
    virtual leaf_layout_requirements get_layout(ui_context& ctx) const = 0;

    virtual void draw(
        ui_context& ctx, layout_box const& region,
        accessor<Value> const& value, widget_state state) const = 0;
};

struct simple_control_data
{
    layout_leaf layout_node;
    themed_rendering_data rendering;
    button_input_state input;
};

template<class Renderer, class DefaultRenderer, class Value>
bool
do_simple_control(
    ui_context& ctx,
    accessor<Value> const& value,
    layout const& layout_spec,
    simple_control_flag_set flags,
    widget_id id,
    simple_control_data* data_ptr = 0)
{
    if (!data_ptr)
        get_cached_data(ctx, &data_ptr);
    simple_control_data& data = *data_ptr;

    init_optional_widget_id(id, &data);

    Renderer const* renderer;
    static DefaultRenderer default_renderer;
    get_themed_renderer(ctx, data.rendering, &renderer, &default_renderer);

    alia_untracked_switch (ctx.event->category)
    {
     alia_untracked_case (REFRESH_CATEGORY):
      {
        leaf_layout_requirements layout_requirements;
        alia_tracked_block (data.rendering.refresh_block)
        {
            layout_requirements = renderer->get_layout(ctx);
        }
        alia_end
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec, layout_requirements,
            LEFT | BASELINE_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     alia_untracked_case (REGION_CATEGORY):
        do_box_region(ctx, id, data.layout_node.assignment().region);
        break;

     alia_untracked_case (INPUT_CATEGORY):
        if (do_button_input(ctx, id, data.input))
            if (flags & SIMPLE_CONTROL_DISABLED)
                return false;
            else
                return true;
        break;
    }
    alia_end

    alia_tracked_block (data.rendering.drawing_block)
    {
        widget_state state = (flags & SIMPLE_CONTROL_DISABLED) ? WIDGET_DISABLED :
            get_button_state(ctx, id, data.input);
        renderer->draw(ctx,
            data.layout_node.assignment().region, value,
            state);
    }
    alia_end

    return false;
}

struct simple_button_renderer : dispatch_interface
{
    virtual leaf_layout_requirements get_layout(ui_context& ctx) const = 0;

    virtual void draw(
        ui_context& ctx, layout_box const& region,
        widget_state state) const = 0;
};

struct simple_button_data
{
    layout_leaf layout_node;
    themed_rendering_data rendering;
    button_input_state input;
};

template<class Renderer, class DefaultRenderer>
bool
do_simple_button(
    ui_context& ctx,
    layout const& layout_spec,
    widget_id id,
    simple_button_data* data_ptr = 0)
{
    if (!data_ptr)
        get_cached_data(ctx, &data_ptr);
    simple_button_data& data = *data_ptr;

    init_optional_widget_id(id, &data);

    Renderer const* renderer;
    static DefaultRenderer default_renderer;
    get_themed_renderer(ctx, data.rendering, &renderer, &default_renderer);

    alia_untracked_switch (ctx.event->category)
    {
     alia_untracked_case (REFRESH_CATEGORY):
      {
        leaf_layout_requirements layout_requirements;
        alia_tracked_block (data.rendering.refresh_block)
        {
            layout_requirements = renderer->get_layout(ctx);
        }
        alia_end
        data.layout_node.refresh_layout(
            get_layout_traversal(ctx), layout_spec, layout_requirements,
            LEFT | BASELINE_Y | PADDED);
        add_layout_node(get_layout_traversal(ctx), &data.layout_node);
        break;
      }

     alia_untracked_case (REGION_CATEGORY):
        do_box_region(ctx, id, data.layout_node.assignment().region);
        break;

     alia_untracked_case (INPUT_CATEGORY):
        if (do_button_input(ctx, id, data.input))
            return true;
        break;
    }
    alia_end

    alia_tracked_block (data.rendering.drawing_block)
    {
        renderer->draw(ctx,
            data.layout_node.assignment().region,
            get_button_state(ctx, id, data.input));
    }
    alia_end

    return false;
}

struct stateless_control_style_path_storage
{
    style_path_storage storage[2];
};

style_search_path const*
get_control_style_path(dataless_ui_context& ctx,
    stateless_control_style_path_storage* storage,
    char const* control_type);

struct control_style_path_storage
{
    stateful_style_path_storage storage[2];
};

style_search_path const*
get_control_style_path(
    dataless_ui_context& ctx, control_style_path_storage* storage,
    char const* control_type, widget_state state);

struct control_style_properties
{
    rgba8 bg_color, fg_color, border_color;
    resolved_box_corner_sizes border_radii;
    float border_width;
};

control_style_properties
get_control_style_properties(
    dataless_ui_context& ctx, style_search_path const* path,
    layout_vector const& size);

control_style_properties
get_control_style_properties(
    dataless_ui_context& ctx, char const* control_type, widget_state state,
    layout_vector const& size);

leaf_layout_requirements
get_box_control_layout(ui_context& ctx, char const* control_type);

skia_box
get_box_control_content_region(
    layout_box const& region, control_style_properties const& style);

void draw_box_control(
    dataless_ui_context& ctx, SkCanvas& canvas, layout_vector const& size,
    control_style_properties const& style, bool has_focus);

void initialize_caching_control_renderer(
    ui_context& ctx, caching_renderer& cache,
    layout_box const& region, id_interface const& content_id);

struct box_control_renderer
{
    box_control_renderer(
        ui_context& ctx, caching_renderer& cache,
        char const* control_type, widget_state state);

    ~box_control_renderer() {}

    void cache() { renderer_.cache(); }

    SkCanvas& canvas() { return renderer_.canvas(); }

    skia_box const& content_region() const { return content_region_; }

    control_style_properties const& style() const { return style_; }

    style_search_path const* style_path() const { return style_path_; }

 private:
    skia_renderer renderer_;
    skia_box content_region_;
    control_style_path_storage path_storage_;
    style_search_path const* style_path_;
    control_style_properties style_;
};

}

#endif
