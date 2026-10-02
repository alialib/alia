#pragma once

#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/context.h>
#include <alia/ui/layout/container_options.hpp>
#include <alia/ui/layout/options.hpp>

#include <utility>

namespace alia {

namespace impl {

template<class Content>
void
consume_provided_box(
    context& ctx, container_options const& options, Content&& content)
{
    if (options.has_provide_box() && !is_refresh_event(ctx))
        *options.box = alia_layout_consume_box(&ctx);
    std::forward<Content>(content)();
}

template<class Begin, class End, class Content>
void
gapped_layout_container(
    context& ctx,
    Begin&& begin,
    End&& end,
    container_options const& options,
    layout_flag_set flags,
    Content&& content)
{
    std::forward<Begin>(begin)(
        &ctx, container_begin_flags(flags, options), options.gap_value);
    consume_provided_box(ctx, options, std::forward<Content>(content));
    std::forward<End>(end)(&ctx);
}

template<class Begin, class End, class Content>
void
flow_layout_container(
    context& ctx,
    Begin&& begin,
    End&& end,
    container_options const& options,
    layout_flag_set flags,
    Content&& content)
{
    std::forward<Begin>(begin)(
        &ctx,
        container_begin_flags(flags, options),
        options.gap_value,
        options.line_gap_value,
        options.minimum_line_height_value);
    consume_provided_box(ctx, options, std::forward<Content>(content));
    std::forward<End>(end)(&ctx);
}

template<class Begin, class End, class Content>
void
simple_layout_container(
    context& ctx,
    Begin&& begin,
    End&& end,
    container_options const& options,
    layout_flag_set flags,
    Content&& content)
{
    std::forward<Begin>(begin)(&ctx, container_begin_flags(flags, options));
    consume_provided_box(ctx, options, std::forward<Content>(content));
    std::forward<End>(end)(&ctx);
}

} // namespace impl

// COMPOSITION CONTAINERS

template<
    container_like Container = container_options,
    layout_like Layout = layout_options,
    class Content>
void
row(context& ctx, Container container, Layout layout, Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        impl::gapped_layout_container(
            ctx,
            alia_layout_row_begin,
            alia_layout_row_end,
            as_container_options(container),
            flags,
            std::forward<Content>(content));
    });
}

template<layout_like Layout, class Content>
    requires(!container_like<Layout>)
void
row(context& ctx, Layout layout, Content&& content)
{
    row(ctx, default_container, layout, std::forward<Content>(content));
}

template<container_like Container, class Content>
    requires(!layout_like<Container>)
void
row(context& ctx, Container container, Content&& content)
{
    row(ctx, container, default_layout, std::forward<Content>(content));
}

template<class Content>
void
row(context& ctx, Content&& content)
{
    row(ctx,
        default_container,
        default_layout,
        std::forward<Content>(content));
}

template<
    container_like Container = container_options,
    layout_like Layout = layout_options,
    class Content>
void
column(context& ctx, Container container, Layout layout, Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        impl::gapped_layout_container(
            ctx,
            alia_layout_column_begin,
            alia_layout_column_end,
            as_container_options(container),
            flags,
            std::forward<Content>(content));
    });
}

template<layout_like Layout, class Content>
    requires(!container_like<Layout>)
void
column(context& ctx, Layout layout, Content&& content)
{
    column(ctx, default_container, layout, std::forward<Content>(content));
}

template<container_like Container, class Content>
    requires(!layout_like<Container>)
void
column(context& ctx, Container container, Content&& content)
{
    column(ctx, container, default_layout, std::forward<Content>(content));
}

template<class Content>
void
column(context& ctx, Content&& content)
{
    column(
        ctx,
        default_container,
        default_layout,
        std::forward<Content>(content));
}

template<
    container_like Container = container_options,
    layout_like Layout = layout_options,
    class Content>
void
zstack(context& ctx, Container container, Layout layout, Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        impl::simple_layout_container(
            ctx,
            alia_layout_zstack_begin,
            alia_layout_zstack_end,
            as_container_options(container),
            flags,
            std::forward<Content>(content));
    });
}

template<layout_like Layout, class Content>
    requires(!container_like<Layout>)
void
zstack(context& ctx, Layout layout, Content&& content)
{
    zstack(ctx, default_container, layout, std::forward<Content>(content));
}

template<container_like Container, class Content>
    requires(!layout_like<Container>)
void
zstack(context& ctx, Container container, Content&& content)
{
    zstack(ctx, container, default_layout, std::forward<Content>(content));
}

template<class Content>
void
zstack(context& ctx, Content&& content)
{
    zstack(
        ctx,
        default_container,
        default_layout,
        std::forward<Content>(content));
}

template<
    container_like Container = container_options,
    layout_like Layout = layout_options,
    class Content>
void
flow(context& ctx, Container container, Layout layout, Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        impl::flow_layout_container(
            ctx,
            alia_layout_flow_begin,
            alia_layout_flow_end,
            as_container_options(container),
            flags,
            std::forward<Content>(content));
    });
}

template<layout_like Layout, class Content>
    requires(!container_like<Layout>)
void
flow(context& ctx, Layout layout, Content&& content)
{
    flow(ctx, default_container, layout, std::forward<Content>(content));
}

template<container_like Container, class Content>
    requires(!layout_like<Container>)
void
flow(context& ctx, Container container, Content&& content)
{
    flow(ctx, container, default_layout, std::forward<Content>(content));
}

template<class Content>
void
flow(context& ctx, Content&& content)
{
    flow(
        ctx,
        default_container,
        default_layout,
        std::forward<Content>(content));
}

template<
    container_like Container = container_options,
    layout_like Layout = layout_options,
    class Content>
void
block_flow(context& ctx, Container container, Layout layout, Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        impl::flow_layout_container(
            ctx,
            alia_layout_block_flow_begin,
            alia_layout_block_flow_end,
            as_container_options(container),
            flags,
            std::forward<Content>(content));
    });
}

template<layout_like Layout, class Content>
    requires(!container_like<Layout>)
void
block_flow(context& ctx, Layout layout, Content&& content)
{
    block_flow(ctx, default_container, layout, std::forward<Content>(content));
}

template<container_like Container, class Content>
    requires(!layout_like<Container>)
void
block_flow(context& ctx, Container container, Content&& content)
{
    block_flow(ctx, container, default_layout, std::forward<Content>(content));
}

template<class Content>
void
block_flow(context& ctx, Content&& content)
{
    block_flow(
        ctx,
        default_container,
        default_layout,
        std::forward<Content>(content));
}

template<layout_like Layout = layout_options, class Content>
void
grid(context& ctx, Layout layout, Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        alia_layout_grid_handle handle
            = alia_layout_grid_begin(&ctx, raw_code(flags));
        std::forward<Content>(content)(handle);
        alia_layout_grid_end(&ctx);
    });
}

template<class Content>
void
grid(context& ctx, Content&& content)
{
    grid(ctx, default_layout, std::forward<Content>(content));
}

template<layout_like Layout = layout_options, class Content>
void
grid_row(
    context& ctx,
    alia_layout_grid_handle grid,
    Layout layout,
    Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        alia_layout_grid_row_begin(&ctx, grid, raw_code(flags));
        std::forward<Content>(content)();
        alia_layout_grid_row_end(&ctx);
    });
}

template<class Content>
void
grid_row(context& ctx, alia_layout_grid_handle grid, Content&& content)
{
    grid_row(ctx, grid, default_layout, std::forward<Content>(content));
}

// WRAPPERS

template<class Content>
void
alignment_override(context& ctx, layout_flag_set flags, Content&& content)
{
    alia_layout_alignment_override_begin(&ctx, raw_code(flags));
    std::forward<Content>(content)();
    alia_layout_alignment_override_end(&ctx);
}

template<class Content>
void
edge_offsets(context& ctx, alia_edge_offsets offsets, Content&& content)
{
    alia_layout_edge_offsets_begin(&ctx, offsets, 0);
    std::forward<Content>(content)();
    alia_layout_edge_offsets_end(&ctx);
}

template<class Content>
void
edge_offsets(
    context& ctx,
    alia_edge_offsets offsets,
    layout_flag_set flags,
    Content&& content)
{
    alia_layout_edge_offsets_begin(&ctx, offsets, raw_code(flags));
    std::forward<Content>(content)();
    alia_layout_edge_offsets_end(&ctx);
}

template<class Content>
void
min_size_constraint(context& ctx, alia_vec2f min_size, Content&& content)
{
    alia_layout_min_size_begin(&ctx, min_size);
    std::forward<Content>(content)();
    alia_layout_min_size_end(&ctx);
}

// Floor the child's size using (length, breadth) relative to the parent's
// main/cross axes.
template<class Content>
void
min_axis_size_constraint(context& ctx, alia_vec2f min_lb, Content&& content)
{
    alia_layout_min_axis_size_begin(&ctx, min_lb);
    std::forward<Content>(content)();
    alia_layout_min_axis_size_end(&ctx);
}

// Cap the child's assigned size and align leftover space (default CENTER).
// A `max_size` component `<= 0` means no cap on that axis.
template<layout_like Layout = layout_options, class Content>
void
clamped(context& ctx, alia_vec2f max_size, Layout layout, Content&& content)
{
    apply_layout(ctx, layout, [&](layout_flag_set flags) {
        flags = add_default_alignment(flags, CENTER_X, CENTER_Y);
        alia_layout_clamped_begin(&ctx, max_size, raw_code(flags));
        std::forward<Content>(content)();
        alia_layout_clamped_end(&ctx);
    });
}

template<class Content>
void
clamped(context& ctx, alia_vec2f max_size, Content&& content)
{
    clamped(ctx, max_size, default_layout, std::forward<Content>(content));
}

// Invoke a width-only clampoed layout..
template<layout_like Layout = layout_options, class Content>
void
clamped(context& ctx, float max_width, Layout layout, Content&& content)
{
    clamped(
        ctx,
        alia_vec2f{max_width, 0.f},
        layout,
        std::forward<Content>(content));
}

template<class Content>
void
clamped(context& ctx, float max_width, Content&& content)
{
    clamped(ctx, max_width, default_layout, std::forward<Content>(content));
}

template<class Content>
void
growth_override(context& ctx, float growth, Content&& content)
{
    alia_layout_growth_override_begin(&ctx, growth);
    std::forward<Content>(content)();
    alia_layout_growth_override_end(&ctx);
}

inline void
flow_spring(context& ctx, float min_width = 0.f)
{
    if (is_refresh_event(ctx))
        alia_layout_flow_spring_emit(&ctx, min_width);
    else
        (void) alia_layout_consume_box(&ctx);
}

// Emit an empty leaf that reserves space.
// While the spacer always takes up a slot in its container, its size is 0x0 by
// default. Like all widgets, it can be explicitly sized using `width`/`height`
// (for sizing along X/Y axes) or `length`/`breadth` (for sizing relative to
// the parent container's axes). By default, it is FLUSH.
template<layout_like Layout = layout_options>
void
spacer(context& ctx, Layout layout = {})
{
    alia_vec2f const size = layout_content_size(layout);
    apply_layout(
        ctx, without_size(std::move(layout)), [&](layout_flag_set flags) {
            flags = add_default_flush(flags);
            if (is_refresh_event(ctx))
            {
                alia_layout_leaf_emit(
                    &ctx,
                    alia_layout_content_metrics_make(
                        alia_vec2f{size.x, size.y}),
                    raw_code(flags));
            }
            else
            {
                (void) alia_layout_consume_box(&ctx);
            }
        });
}

} // namespace alia
