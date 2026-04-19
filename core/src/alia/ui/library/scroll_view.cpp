#include <alia/abi/ui/library.h>

#include <algorithm>
#include <cstdint>

#include <alia/abi/kernel/routing.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/kernel/timing.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/input/keyboard.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/utilities.h>
#include <alia/abi/ui/palette.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>

using namespace alia::operators;

namespace alia {

static constexpr uint8_t ALIA_SCROLL_AXIS_X = 1u << 0;
static constexpr uint8_t ALIA_SCROLL_AXIS_Y = 1u << 1;

struct scroll_view_state;

struct scroll_view_layout_node
{
    alia_layout_container base;
    scroll_view_state* data;
};

struct scroll_view_placement
{
    alia_box viewport_box;
    alia_vec2f content_size;
    alia_vec2f view_size;
    alia_box scrollbar_area[2];
    bool scrollbar_on[2];
};

struct scrollbar_data
{
    float logical = 0.f;
    float smoothed = 0.f;
    float physical = 0.f;
    float drag_start_delta = 0.f;
    alia_timer_state repeat_timer = {};
    int8_t repeat_direction = 0;
};

struct scroll_view_state
{
    scrollbar_data bar[2];
    uint8_t scrollable_axes = ALIA_SCROLL_AXIS_X | ALIA_SCROLL_AXIS_Y;
    uint8_t reserved_axes = 0;
    alia_layout_flags_t layout_flags = 0;
    alia_scrollbar_style style = {};
    alia_vec2f content_size = {0.f, 0.f};
    alia_vec2f view_size = {0.f, 0.f};
    bool scrollbars_on[2] = {false, false};
};

static alia_scrollbar_style const default_scrollbar_style = {
    .track_color = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_BACKGROUND,
            ALIA_PALETTE_RAMP_LEVEL_BASE),
        0xff),
    .thumb_color = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_BACKGROUND,
            ALIA_PALETTE_RAMP_LEVEL_STRONGER_4),
        0xff),
    .button_color = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_BACKGROUND,
            ALIA_PALETTE_RAMP_LEVEL_STRONGER_2),
        0xff),
    .glyph_color = alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_BACKGROUND,
            ALIA_PALETTE_RAMP_LEVEL_STRONGER_4),
        0xff),
    .highlight = alia_palette_color_make(
        alia_palette_index_swatch(
            ALIA_PALETTE_SWATCH_PRIMARY, ALIA_PALETTE_SWATCH_PART_SUBTLE),
        0x40),
    .width = 24.f,
    .button_length = 0.f,
    .minimum_thumb_length = 20.f,
    .thumb_corner_radius = 0.f,
    .line_size = 72.f,
    .scroll_input_scale = 120.f,
};

static inline float
axis_of(alia_vec2f const& v, unsigned axis)
{
    return axis == 0 ? v.x : v.y;
}

static inline float&
axis_of(alia_vec2f& v, unsigned axis)
{
    return axis == 0 ? v.x : v.y;
}

static inline float
clamp_float(float x, float lo, float hi)
{
    return (std::max) (lo, (std::min) (x, hi));
}

static inline float
max_logical(scroll_view_placement const& p, unsigned axis)
{
    return (
        std::max) (0.f,
                   axis_of(p.content_size, axis) - axis_of(p.view_size, axis));
}

static float
clamp_logical(scroll_view_placement const& p, unsigned axis, float logical)
{
    return clamp_float(logical, 0.f, max_logical(p, axis));
}

static float
max_physical(
    scroll_view_state const& data,
    scroll_view_placement const& p,
    unsigned axis)
{
    float const button_len = data.style.button_length;
    float const area_len = axis == 0 ? p.scrollbar_area[axis].size.x
                                     : p.scrollbar_area[axis].size.y;
    float const bg_len = area_len - 2.f * button_len;
    if (bg_len <= 0.f)
        return 0.f;
    float const content = axis_of(p.content_size, axis);
    float const view = axis_of(p.view_size, axis);
    if (content <= 0.f)
        return 0.f;
    float const thumb_len = (std::max) (data.style.minimum_thumb_length,
                                        view * bg_len / content);
    return (std::max) (0.f, bg_len - thumb_len);
}

static float
logical_to_physical(
    scroll_view_state const& data,
    scroll_view_placement const& p,
    unsigned axis,
    float logical)
{
    float const max_l = max_logical(p, axis);
    float const max_p = max_physical(data, p, axis);
    if (max_l <= 0.f || max_p <= 0.f)
        return 0.f;
    return clamp_float(logical * max_p / max_l, 0.f, max_p);
}

static float
physical_to_logical(
    scroll_view_state const& data,
    scroll_view_placement const& p,
    unsigned axis,
    float physical)
{
    float const max_l = max_logical(p, axis);
    float const max_p = max_physical(data, p, axis);
    if (max_l <= 0.f || max_p <= 0.f)
        return 0.f;
    return clamp_float(physical * max_l / max_p, 0.f, max_l);
}

static void
set_logical(
    scroll_view_state& data,
    scroll_view_placement const& p,
    unsigned axis,
    float logical)
{
    auto& bar = data.bar[axis];
    bar.logical = clamp_logical(p, axis, logical);
    bar.physical = logical_to_physical(data, p, axis, bar.logical);
    bar.smoothed = bar.logical;
}

static alia_box
thumb_area(
    scroll_view_state const& data,
    scroll_view_placement const& p,
    unsigned axis)
{
    alia_box area = p.scrollbar_area[axis];
    float const button_len = data.style.button_length;
    float const bg_len
        = (axis == 0 ? area.size.x : area.size.y) - 2.f * button_len;
    float const content = axis_of(p.content_size, axis);
    float const view = axis_of(p.view_size, axis);
    float const length
        = content <= 0.f ? bg_len
                         : (std::max) (data.style.minimum_thumb_length,
                                       view * bg_len / content);
    if (axis == 0)
    {
        area.min.x += button_len + data.bar[axis].physical;
        area.size.x = length;
    }
    else
    {
        area.min.y += button_len + data.bar[axis].physical;
        area.size.y = length;
    }
    return area;
}

static alia_box
button_area(
    scroll_view_state const& data,
    scroll_view_placement const& p,
    unsigned axis,
    bool hi)
{
    alia_box area = p.scrollbar_area[axis];
    float const len = data.style.button_length;
    if (axis == 0)
    {
        area.size.x = len;
        if (hi)
            area.min.x += p.scrollbar_area[axis].size.x - len;
    }
    else
    {
        area.size.y = len;
        if (hi)
            area.min.y += p.scrollbar_area[axis].size.y - len;
    }
    return area;
}

static alia_box
page_area(
    scroll_view_state const& data,
    scroll_view_placement const& p,
    unsigned axis,
    bool after_thumb)
{
    alia_box area = p.scrollbar_area[axis];
    alia_box thumb = thumb_area(data, p, axis);
    float const button_len = data.style.button_length;
    if (axis == 0)
    {
        float const bg_start = area.min.x + button_len;
        float const bg_end = area.min.x + area.size.x - button_len;
        if (!after_thumb)
        {
            area.min.x = bg_start;
            area.size.x = (std::max) (0.f, thumb.min.x - bg_start);
        }
        else
        {
            float const start = thumb.min.x + thumb.size.x;
            area.min.x = start;
            area.size.x = (std::max) (0.f, bg_end - start);
        }
    }
    else
    {
        float const bg_start = area.min.y + button_len;
        float const bg_end = area.min.y + area.size.y - button_len;
        if (!after_thumb)
        {
            area.min.y = bg_start;
            area.size.y = (std::max) (0.f, thumb.min.y - bg_start);
        }
        else
        {
            float const start = thumb.min.y + thumb.size.y;
            area.min.y = start;
            area.size.y = (std::max) (0.f, bg_end - start);
        }
    }
    return area;
}

static alia_element_id
scroll_id(alia_element_id base, unsigned axis, unsigned slot)
{
    return alia_offset_id(base, 1 + axis * 8 + slot);
}

static void
scroll_view_handle_key(
    scroll_view_state& data,
    scroll_view_placement const& p,
    alia_modded_key const& key)
{
    if (key.mods != 0)
        return;
    float const line = data.style.line_size;
    switch (key.code)
    {
        case ALIA_KEY_UP:
            set_logical(data, p, 1, data.bar[1].logical - line);
            break;
        case ALIA_KEY_DOWN:
            set_logical(data, p, 1, data.bar[1].logical + line);
            break;
        case ALIA_KEY_LEFT:
            set_logical(data, p, 0, data.bar[0].logical - line);
            break;
        case ALIA_KEY_RIGHT:
            set_logical(data, p, 0, data.bar[0].logical + line);
            break;
        case ALIA_KEY_PAGE_UP:
            set_logical(
                data, p, 1, data.bar[1].logical - axis_of(p.view_size, 1));
            break;
        case ALIA_KEY_PAGE_DOWN:
            set_logical(
                data, p, 1, data.bar[1].logical + axis_of(p.view_size, 1));
            break;
        case ALIA_KEY_HOME:
            set_logical(data, p, 1, 0.f);
            break;
        case ALIA_KEY_END:
            set_logical(data, p, 1, max_logical(p, 1));
            break;
        default:
            break;
    }
}

static void
draw_scrollbar(
    alia_context* ctx,
    alia_element_id base_id,
    scroll_view_state& data,
    scroll_view_placement const& p,
    unsigned axis)
{
    alia_palette const* palette = alia_ctx_palette(ctx);
    alia_box const sb = p.scrollbar_area[axis];
    alia_box const thumb = thumb_area(data, p, axis);
    alia_box const btn0 = button_area(data, p, axis, false);
    alia_box const btn1 = button_area(data, p, axis, true);
    alia_interaction_status_t const thumb_status
        = alia_element_get_interaction_status(
            ctx, scroll_id(base_id, axis, 0), 0);

    alia_draw_rounded_box(
        ctx,
        ctx->geometry->z_base + 1,
        sb,
        alia_palette_color_resolve(palette, data.style.track_color),
        0.f);

    alia_draw_rounded_box(
        ctx,
        ctx->geometry->z_base + 2,
        thumb,
        alia_palette_color_resolve(palette, data.style.thumb_color),
        data.style.thumb_corner_radius);

    if ((thumb_status
         & (ALIA_INTERACTION_STATUS_ACTIVE | ALIA_INTERACTION_STATUS_HOVERED))
        != 0)
    {
        alia_draw_rounded_box(
            ctx,
            ctx->geometry->z_base + 2,
            thumb,
            alia_palette_color_resolve(palette, data.style.highlight),
            data.style.thumb_corner_radius);
    }

    alia_srgba8 const button
        = alia_palette_color_resolve(palette, data.style.button_color);
    alia_draw_rounded_box(ctx, ctx->geometry->z_base + 2, btn0, button, 0.f);
    alia_draw_rounded_box(ctx, ctx->geometry->z_base + 2, btn1, button, 0.f);

    float const rotation0 = axis == 0 ? 270.f : 0.f;
    float const rotation1 = axis == 0 ? 90.f : 180.f;
    alia_srgba8 const glyph
        = alia_palette_color_resolve(palette, data.style.glyph_color);
    alia_draw_equilateral_triangle(
        ctx, ctx->geometry->z_base + 3, btn0, glyph, rotation0);
    alia_draw_equilateral_triangle(
        ctx, ctx->geometry->z_base + 3, btn1, glyph, rotation1);
}

static void
do_scrollbar_pass(
    alia_context* ctx,
    alia_element_id base_id,
    scroll_view_state& data,
    scroll_view_placement const& p,
    unsigned axis)
{
    if (!p.scrollbar_on[axis])
        return;

    auto& bar = data.bar[axis];
    alia_element_id const thumb_id = scroll_id(base_id, axis, 0);
    alia_element_id const btn0_id = scroll_id(base_id, axis, 1);
    alia_element_id const btn1_id = scroll_id(base_id, axis, 2);
    alia_element_id const page0_id = scroll_id(base_id, axis, 3);
    alia_element_id const page1_id = scroll_id(base_id, axis, 4);

    switch (get_event_category(*ctx))
    {
        case ALIA_CATEGORY_SPATIAL: {
            alia_box thumb = thumb_area(data, p, axis);
            alia_box btn0 = button_area(data, p, axis, false);
            alia_box btn1 = button_area(data, p, axis, true);
            alia_box page0 = page_area(data, p, axis, false);
            alia_box page1 = page_area(data, p, axis, true);
            alia_element_box_region(
                ctx, thumb_id, &thumb, ALIA_CURSOR_DEFAULT);
            alia_element_box_region(ctx, btn0_id, &btn0, ALIA_CURSOR_DEFAULT);
            alia_element_box_region(ctx, btn1_id, &btn1, ALIA_CURSOR_DEFAULT);
            alia_element_box_region(
                ctx, page0_id, &page0, ALIA_CURSOR_DEFAULT);
            alia_element_box_region(
                ctx, page1_id, &page1, ALIA_CURSOR_DEFAULT);
            break;
        }

        case ALIA_CATEGORY_INPUT: {
            if (alia_element_detect_mouse_press(
                    ctx, btn0_id, ALIA_BUTTON_LEFT))
                set_logical(data, p, axis, bar.logical - data.style.line_size);
            if (alia_element_detect_mouse_press(
                    ctx, btn1_id, ALIA_BUTTON_LEFT))
                set_logical(data, p, axis, bar.logical + data.style.line_size);

            float const page = axis_of(p.view_size, axis);
            if (alia_element_detect_mouse_press(
                    ctx, page0_id, ALIA_BUTTON_LEFT))
                set_logical(data, p, axis, bar.logical - page);
            if (alia_element_detect_mouse_press(
                    ctx, page1_id, ALIA_BUTTON_LEFT))
                set_logical(data, p, axis, bar.logical + page);

            if (alia_element_detect_mouse_press(
                    ctx, thumb_id, ALIA_BUTTON_LEFT))
            {
                alia_vec2f const pointer = alia_input_pointer_position(ctx);
                float const pointer_axis = axis == 0 ? pointer.x : pointer.y;
                float const thumb_min
                    = axis == 0 ? thumb_area(data, p, axis).min.x
                                : thumb_area(data, p, axis).min.y;
                bar.drag_start_delta = thumb_min - pointer_axis;
            }
            if (alia_element_detect_drag(ctx, thumb_id, ALIA_BUTTON_LEFT))
            {
                alia_vec2f const pointer = alia_input_pointer_position(ctx);
                float const pointer_axis = axis == 0 ? pointer.x : pointer.y;
                float const sb_min = axis == 0 ? p.scrollbar_area[axis].min.x
                                               : p.scrollbar_area[axis].min.y;
                float const physical = pointer_axis + bar.drag_start_delta
                                     - sb_min - data.style.button_length;
                bar.physical
                    = clamp_float(physical, 0.f, max_physical(data, p, axis));
                bar.logical = physical_to_logical(data, p, axis, bar.physical);
                bar.smoothed = bar.logical;
            }
            break;
        }

        case ALIA_CATEGORY_DRAWING:
            if (!alia_element_is_drag_in_progress(
                    ctx, thumb_id, ALIA_BUTTON_LEFT))
                bar.physical = logical_to_physical(data, p, axis, bar.logical);
            draw_scrollbar(ctx, base_id, data, p, axis);
            break;

        default:
            break;
    }
}

static void
handle_make_widget_visible(
    scroll_view_state& data,
    scroll_view_placement const& p,
    alia_make_widget_visible const& e)
{
    alia_box const region = e.region;
    float const r_min[2] = {region.min.x, region.min.y};
    float const r_max[2]
        = {region.min.x + region.size.x, region.min.y + region.size.y};
    float const v_min[2]
        = {p.viewport_box.min.x + data.bar[0].logical,
           p.viewport_box.min.y + data.bar[1].logical};
    float const v_max[2]
        = {v_min[0] + p.view_size.x, v_min[1] + p.view_size.y};
    for (unsigned axis = 0; axis != 2; ++axis)
    {
        float correction = 0.f;
        if (r_min[axis] < v_min[axis])
            correction = r_min[axis] - v_min[axis];
        else if (r_max[axis] > v_max[axis])
            correction = r_max[axis] - v_max[axis];
        if (correction != 0.f)
            set_logical(data, p, axis, data.bar[axis].logical + correction);
    }
}

alia_horizontal_requirements
scroll_view_measure_horizontal(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    auto& n = *reinterpret_cast<scroll_view_layout_node*>(node);
    auto& d = *n.data;
    alia_horizontal_requirements child = {0.f, 0.f};
    if (n.base.first_child)
        child = alia_measure_horizontal(ctx, n.base.first_child);

    float required = child.min_size;
    if (d.scrollable_axes & ALIA_SCROLL_AXIS_X)
        required = d.style.minimum_thumb_length;
    if (d.reserved_axes & ALIA_SCROLL_AXIS_Y)
        required += d.style.width;

    d.content_size.x = child.min_size;
    return {
        .min_size = required,
        .growth_factor = alia_resolve_growth_factor(n.base.flags)};
}

void
scroll_view_assign_widths(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& n = *reinterpret_cast<scroll_view_layout_node*>(node);
    auto& d = *n.data;
    float child_width = assigned_width;
    if (d.scrollbars_on[1] || (d.reserved_axes & ALIA_SCROLL_AXIS_Y))
        child_width = (std::max) (0.f, child_width - d.style.width);
    if (n.base.first_child)
        alia_assign_widths(ctx, main_axis, n.base.first_child, child_width);
}

alia_vertical_requirements
scroll_view_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    auto& n = *reinterpret_cast<scroll_view_layout_node*>(node);
    auto& d = *n.data;
    float child_width = assigned_width;
    if (d.scrollbars_on[1] || (d.reserved_axes & ALIA_SCROLL_AXIS_Y))
        child_width = (std::max) (0.f, child_width - d.style.width);

    alia_vertical_requirements child = {0.f, 0.f, 0.f, 0.f};
    if (n.base.first_child)
        child = alia_measure_vertical(
            ctx, main_axis, n.base.first_child, child_width);

    float required = child.min_size;
    if (d.scrollable_axes & ALIA_SCROLL_AXIS_Y)
        required = d.style.minimum_thumb_length;

    d.content_size.y = child.min_size;
    return {
        .min_size = required,
        .growth_factor = alia_resolve_growth_factor(n.base.flags),
        .ascent = child.ascent,
        .descent = required - child.ascent};
}

void
scroll_view_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& n = *reinterpret_cast<scroll_view_layout_node*>(node);
    auto& d = *n.data;

    bool vertical_on = (d.reserved_axes & ALIA_SCROLL_AXIS_Y) != 0;
    bool horizontal_on = (d.reserved_axes & ALIA_SCROLL_AXIS_X) != 0;
    alia_vec2f view_size = box.size;

    if (vertical_on)
        view_size.x -= d.style.width;
    if (horizontal_on)
        view_size.y -= d.style.width;

    bool changed = true;
    for (int i = 0; i != 2 && changed; ++i)
    {
        changed = false;

        bool const needs_vertical
            = (d.scrollable_axes & ALIA_SCROLL_AXIS_Y) != 0
           && d.content_size.y > view_size.y;
        if (needs_vertical != vertical_on)
        {
            vertical_on = needs_vertical;
            changed = true;
        }

        bool const needs_horizontal
            = (d.scrollable_axes & ALIA_SCROLL_AXIS_X) != 0
           && d.content_size.x > view_size.x;
        if (needs_horizontal != horizontal_on)
        {
            horizontal_on = needs_horizontal;
            changed = true;
        }

        view_size = box.size;
        if (vertical_on)
            view_size.x -= d.style.width;
        if (horizontal_on)
            view_size.y -= d.style.width;
    }

    d.scrollbars_on[0] = horizontal_on;
    d.scrollbars_on[1] = vertical_on;
    d.view_size = view_size;

    alia_box viewport = box;
    viewport.size = view_size;

    alia_vec2f content_box_size
        = {(std::max) (view_size.x, d.content_size.x),
           (std::max) (view_size.y, d.content_size.y)};

    scroll_view_placement* placement
        = arena_alloc<scroll_view_placement>(ctx->arena);
    placement->viewport_box = viewport;
    placement->content_size = content_box_size;
    placement->view_size = view_size;
    placement->scrollbar_on[0] = horizontal_on;
    placement->scrollbar_on[1] = vertical_on;
    placement->scrollbar_area[0]
        = {.min = {viewport.min.x, viewport.min.y + viewport.size.y},
           .size = {viewport.size.x, d.style.width}};
    placement->scrollbar_area[1]
        = {.min = {viewport.min.x + viewport.size.x, viewport.min.y},
           .size = {d.style.width, viewport.size.y}};
    if (horizontal_on && vertical_on)
    {
        placement->scrollbar_area[0].size.x += d.style.width;
        placement->scrollbar_area[1].size.y += d.style.width;
    }

    if (n.base.first_child)
    {
        alia_assign_boxes(
            ctx,
            main_axis,
            n.base.first_child,
            {.min = viewport.min, .size = content_box_size},
            baseline);
    }
}

alia_layout_node_vtable scroll_view_vtable
    = {scroll_view_measure_horizontal,
       scroll_view_assign_widths,
       scroll_view_measure_vertical,
       scroll_view_assign_boxes,
       scroll_view_measure_horizontal,
       alia_default_measure_wrapped_vertical,
       nullptr};

struct scroll_view_scope
{
    scroll_view_layout_node* node = nullptr;
    scroll_view_state* data = nullptr;
    scroll_view_placement placement = {};
    alia_element_id id = ALIA_ELEMENT_ID_NONE;
};

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

void
alia_ui_scroll_view_begin(
    alia_context* ctx,
    alia_layout_flags_t layout_flags,
    uint8_t scrollable_axes,
    uint8_t reserved_axes,
    alia_scrollbar_style const* style)
{
    auto& scope = stack_push<scroll_view_scope>(ctx);

    alia_substrate_usage_result result = alia_substrate_use_memory(
        ctx, sizeof(scroll_view_state), alignof(scroll_view_state));
    auto* data = reinterpret_cast<scroll_view_state*>(result.ptr);
    if (result.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        new (data) scroll_view_state;
    }
    scope.data = data;
    scope.id = alia_make_element_id(ctx, result);

    data->scrollable_axes = scrollable_axes;
    data->reserved_axes = reserved_axes;
    data->layout_flags = layout_flags;
    data->style = style != nullptr ? *style : default_scrollbar_style;

    if (is_refresh_event(*ctx))
    {
        auto* node = arena_alloc<scroll_view_layout_node>(
            ctx->layout->emission.arena);
        *node = scroll_view_layout_node{
            .base
            = {.base
               = {.vtable = &scroll_view_vtable, .next_sibling = nullptr},
               .flags = layout_flags,
               .first_child = nullptr},
            .data = data};
        scope.node = node;
        alia_layout_container_activate(ctx, &node->base);
        return;
    }

    auto* placement = arena_alloc<scroll_view_placement>(
        *alia_layout_placement_arena(ctx));
    scope.placement = *placement;

    for (unsigned axis = 0; axis != 2; ++axis)
    {
        if (!scope.placement.scrollbar_on[axis])
            set_logical(*data, scope.placement, axis, 0.f);
        else
            set_logical(*data, scope.placement, axis, data->bar[axis].logical);
    }

    if (get_event_category(*ctx) == ALIA_CATEGORY_SPATIAL)
    {
        alia_element_hit_test_box_region(
            ctx,
            scope.id,
            &scope.placement.viewport_box,
            ALIA_HIT_TEST_SCROLL_INPUT,
            ALIA_CURSOR_DEFAULT);
    }

    if (get_event_category(*ctx) == ALIA_CATEGORY_INPUT)
    {
        alia_vec2f scroll_input_delta = {};
        if (alia_element_detect_scroll(ctx, scope.id, &scroll_input_delta))
        {
            if (scope.placement.scrollbar_on[0])
            {
                set_logical(
                    *data,
                    scope.placement,
                    0,
                    data->bar[0].logical
                        - scroll_input_delta.x
                              * data->style.scroll_input_scale);
            }
            if (scope.placement.scrollbar_on[1])
            {
                set_logical(
                    *data,
                    scope.placement,
                    1,
                    data->bar[1].logical
                        - scroll_input_delta.y
                              * data->style.scroll_input_scale);
            }
        }

        alia_element_focus_on_click(ctx, scope.id);
        alia_modded_key key = {};
        if (alia_element_detect_key_press(ctx, scope.id, &key))
            scroll_view_handle_key(*data, scope.placement, key);
    }

    for (unsigned axis = 0; axis != 2; ++axis)
    {
        do_scrollbar_pass(ctx, scope.id, *data, scope.placement, axis);
    }

    alia_geometry_push_clip_box(ctx, scope.placement.viewport_box);
    alia_geometry_push_translation(
        ctx, {-data->bar[0].smoothed, -data->bar[1].smoothed});
}

void
alia_ui_scroll_view_end(alia_context* ctx)
{
    // `scroll_view_begin` pushes, in order: scroll_view_scope, then
    // (non-refresh only) clip, then translation — all on `ctx->stack`.
    // Pops must be LIFO.
    if (is_refresh_event(*ctx))
    {
        auto scope = stack_pop<scroll_view_scope>(ctx);
        if (!scope.data)
            return;
        if (scope.node)
            alia_layout_container_deactivate(ctx, &scope.node->base);
        return;
    }

    alia_geometry_pop_translation(ctx);
    alia_geometry_pop_clip_box(ctx);

    auto scope = stack_pop<scroll_view_scope>(ctx);
    if (!scope.data)
        return;

    if (get_event_type(*ctx) == ALIA_EVENT_MAKE_WIDGET_VISIBLE)
    {
        auto const& e = as_make_widget_visible_event(*ctx);
        if (e.acknowledged)
            handle_make_widget_visible(*scope.data, scope.placement, e);
    }
    else if (get_event_category(*ctx) == ALIA_CATEGORY_INPUT)
    {
        alia_modded_key key = {};
        if (alia_input_detect_key_press(ctx, &key))
            scroll_view_handle_key(*scope.data, scope.placement, key);
    }
}

alia_scrollbar_style const*
alia_default_scrollbar_style(void)
{
    return &default_scrollbar_style;
}

ALIA_EXTERN_C_END
