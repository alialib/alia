#pragma once

#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/context.h>
#include <alia/ui/layout/flags.hpp>

#include <type_traits>
#include <utility>

namespace alia {

// COMPOSITION CONTAINERS

namespace impl {

struct layout_gap
{
    float value;
};

struct layout_line_gap
{
    float value;
};

struct layout_minimum_line_height
{
    float value;
};

struct layout_config
{
    layout_flag_set flags = NO_FLAGS;
    alia_box* box = nullptr;
    layout_gap gap = {0.f};
    layout_line_gap line_gap = {0.f};
    layout_minimum_line_height minimum_line_height = {0.f};

    int
    build_code() const
    {
        int code = raw_code(flags);
        if (box)
            code |= ALIA_PROVIDE_BOX;
        return code;
    }
};

template<typename T>
constexpr bool is_valid_layout_arg_v
    = std::is_same_v<std::decay_t<T>, layout_flag_set>
   || std::is_same_v<std::decay_t<T>, alia_box*>
   || std::is_same_v<std::decay_t<T>, layout_gap>;

template<typename T>
constexpr bool is_valid_flow_layout_arg_v
    = is_valid_layout_arg_v<T>
   || std::is_same_v<std::decay_t<T>, layout_line_gap>
   || std::is_same_v<std::decay_t<T>, layout_minimum_line_height>;

// Counts how many times 'T' appears in the 'Args' pack
template<typename T, typename... Args>
constexpr std::size_t count_type_v
    = (0 + ... + std::is_same_v<T, std::decay_t<Args>>);

template<typename T>
constexpr void
apply_layout_arg(layout_config& config, T&& arg)
{
    using ArgType = std::decay_t<T>;
    if constexpr (std::is_same_v<ArgType, layout_flag_set>)
    {
        config.flags = arg;
    }
    else if constexpr (std::is_same_v<ArgType, alia_box*>)
    {
        config.box = arg;
    }
    else if constexpr (std::is_same_v<ArgType, layout_gap>)
    {
        config.gap = arg;
    }
}

template<typename T>
constexpr void
apply_flow_layout_arg(layout_config& config, T&& arg)
{
    using ArgType = std::decay_t<T>;
    if constexpr (std::is_same_v<ArgType, layout_flag_set>)
    {
        config.flags = arg;
    }
    else if constexpr (std::is_same_v<ArgType, alia_box*>)
    {
        config.box = arg;
    }
    else if constexpr (std::is_same_v<ArgType, layout_gap>)
    {
        config.gap = arg;
    }
    else if constexpr (std::is_same_v<ArgType, layout_line_gap>)
    {
        config.line_gap = arg;
    }
    else if constexpr (std::is_same_v<ArgType, layout_minimum_line_height>)
    {
        config.minimum_line_height = arg;
    }
}

template<typename... Args>
concept layout_args_must_have_maximum_one_flag_set
    = (count_type_v<layout_flag_set, Args...> <= 1);

template<typename... Args>
concept layout_args_must_have_maximum_one_box
    = (count_type_v<alia_box*, Args...> <= 1);

template<typename... Args>
concept layout_args_contain_only_valid_types_and_one_content_block
    = ((0 + ... + is_valid_layout_arg_v<Args>) == sizeof...(Args) - 1);

template<typename... Args>
concept layout_args_must_have_maximum_one_gap
    = (count_type_v<layout_gap, Args...> <= 1);

template<typename... Args>
concept layout_args_must_have_maximum_one_line_gap
    = (count_type_v<layout_line_gap, Args...> <= 1);

template<typename... Args>
concept layout_args_must_have_maximum_one_minimum_line_height
    = (count_type_v<layout_minimum_line_height, Args...> <= 1);

template<typename... Args>
concept ValidLayoutPack
    = sizeof...(Args) > 0
   && layout_args_contain_only_valid_types_and_one_content_block<Args...>&&
          layout_args_must_have_maximum_one_flag_set<Args...>&&
              layout_args_must_have_maximum_one_box<Args...>&&
                  layout_args_must_have_maximum_one_gap<Args...>;

template<class Begin, class End, class... Args>
    requires ValidLayoutPack<Args...>
void
gapped_layout_container(context& ctx, Begin&& begin, End&& end, Args&&... args)
{
    auto&& content = (std::forward<Args>(args), ...);

    layout_config config;
    (apply_layout_arg(config, std::forward<Args>(args)), ...);

    std::forward<Begin>(begin)(&ctx, config.build_code(), config.gap.value);

    if constexpr ((std::is_same_v<alia_box*, std::decay_t<Args>> || ...))
    {
        if (!is_refresh_event(ctx))
            *config.box = alia_layout_consume_box(&ctx);
    }

    content();

    std::forward<End>(end)(&ctx);
}

template<typename... Args>
concept ValidFlowLayoutPack
    = sizeof...(Args) > 0
   && ((0 + ... + is_valid_flow_layout_arg_v<Args>) == sizeof...(Args) - 1)
   && layout_args_must_have_maximum_one_flag_set<Args...>&&
          layout_args_must_have_maximum_one_box<Args...>&&
              layout_args_must_have_maximum_one_gap<Args...>&&
                  layout_args_must_have_maximum_one_line_gap<Args...>&&
                      layout_args_must_have_maximum_one_minimum_line_height<
                          Args...>;

template<class Begin, class End, class... Args>
    requires ValidFlowLayoutPack<Args...>
void
flow_layout_container(context& ctx, Begin&& begin, End&& end, Args&&... args)
{
    auto&& content = (std::forward<Args>(args), ...);

    layout_config config;
    (apply_flow_layout_arg(config, std::forward<Args>(args)), ...);

    std::forward<Begin>(begin)(
        &ctx,
        config.build_code(),
        config.gap.value,
        config.line_gap.value,
        config.minimum_line_height.value);

    if constexpr ((std::is_same_v<alia_box*, std::decay_t<Args>> || ...))
    {
        if (!is_refresh_event(ctx))
            *config.box = alia_layout_consume_box(&ctx);
    }

    content();

    std::forward<End>(end)(&ctx);
}

template<typename... Args>
concept ValidSimpleLayoutPack
    = sizeof...(Args) > 0
   && layout_args_contain_only_valid_types_and_one_content_block<Args...>&&
          layout_args_must_have_maximum_one_flag_set<Args...>&&
              layout_args_must_have_maximum_one_box<Args...>&&
                  layout_args_must_have_maximum_one_gap<Args...>
   && (count_type_v<layout_gap, Args...> == 0);

template<class Begin, class End, class... Args>
    requires ValidSimpleLayoutPack<Args...>
void
simple_layout_container(context& ctx, Begin&& begin, End&& end, Args&&... args)
{
    auto&& content = (std::forward<Args>(args), ...);

    layout_config config;
    (apply_layout_arg(config, std::forward<Args>(args)), ...);

    std::forward<Begin>(begin)(&ctx, config.build_code());

    if constexpr ((std::is_same_v<alia_box*, std::decay_t<Args>> || ...))
    {
        if (!is_refresh_event(ctx))
            *config.box = alia_layout_consume_box(&ctx);
    }

    content();

    std::forward<End>(end)(&ctx);
}

} // namespace impl

inline impl::layout_gap
gap(float gap)
{
    return impl::layout_gap{gap};
}

inline impl::layout_line_gap
line_gap(float line_gap)
{
    return impl::layout_line_gap{line_gap};
}

inline impl::layout_minimum_line_height
minimum_line_height(float minimum_line_height)
{
    return impl::layout_minimum_line_height{minimum_line_height};
}

template<class... ArgPack>
    requires impl::ValidLayoutPack<ArgPack...>
void
row(context& ctx, ArgPack&&... args)
{
    impl::gapped_layout_container(
        ctx,
        alia_layout_row_begin,
        alia_layout_row_end,
        std::forward<ArgPack>(args)...);
}

template<class... ArgPack>
    requires impl::ValidLayoutPack<ArgPack...>
void
column(context& ctx, ArgPack&&... args)
{
    impl::gapped_layout_container(
        ctx,
        alia_layout_column_begin,
        alia_layout_column_end,
        std::forward<ArgPack>(args)...);
}

template<class... ArgPack>
    requires impl::ValidFlowLayoutPack<ArgPack...>
void
flow(context& ctx, ArgPack&&... args)
{
    impl::flow_layout_container(
        ctx,
        alia_layout_flow_begin,
        alia_layout_flow_end,
        std::forward<ArgPack>(args)...);
}

template<class... ArgPack>
    requires impl::ValidFlowLayoutPack<ArgPack...>
void
block_flow(context& ctx, ArgPack&&... args)
{
    impl::flow_layout_container(
        ctx,
        alia_layout_block_flow_begin,
        alia_layout_block_flow_end,
        std::forward<ArgPack>(args)...);
}

template<class Content>
void
grid(context& ctx, Content&& content)
{
    alia_layout_grid_handle handle = alia_layout_grid_begin(&ctx, 0);
    std::forward<Content>(content)(handle);
    alia_layout_grid_end(&ctx);
}

template<class Content>
void
grid(context& ctx, layout_flag_set flags, Content&& content)
{
    alia_layout_grid_handle handle
        = alia_layout_grid_begin(&ctx, raw_code(flags));
    std::forward<Content>(content)(handle);
    alia_layout_grid_end(&ctx);
}

template<class Content>
void
grid_row(context& ctx, alia_layout_grid_handle grid, Content&& content)
{
    alia_layout_grid_row_begin(&ctx, grid, 0);
    std::forward<Content>(content)();
    alia_layout_grid_row_end(&ctx);
}

template<class Content>
void
grid_row(
    context& ctx,
    alia_layout_grid_handle grid,
    layout_flag_set flags,
    Content&& content)
{
    alia_layout_grid_row_begin(&ctx, grid, raw_code(flags));
    std::forward<Content>(content)();
    alia_layout_grid_row_end(&ctx);
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

// Emit an empty leaf that reserves `size` (logical px, theme-scaled).
// FLUSH is always applied so style spacing does not inflate the reserved size.
inline void
spacer(context& ctx, alia_vec2f size, layout_flag_set flags = NO_FLAGS)
{
    if (is_refresh_event(ctx))
    {
        alia_layout_leaf_emit(
            &ctx,
            alia_layout_content_metrics_make(alia_vec2f{
                alia_px(&ctx, size.x), alia_px(&ctx, size.y)}),
            raw_code(flags | FLUSH));
    }
    else
        (void) alia_layout_consume_box(&ctx);
}

} // namespace alia
