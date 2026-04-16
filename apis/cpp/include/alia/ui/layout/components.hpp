#pragma once

#include <alia/abi/ui/layout/components.h>
#include <alia/context.h>
#include <alia/ui/layout/flags.hpp>

#include <type_traits>
#include <utility>

namespace alia {

// COMPOSITION CONTAINERS

namespace impl {

struct layout_config
{
    layout_flag_set flags = NO_FLAGS;
    alia_box* box = nullptr;

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
   || std::is_same_v<std::decay_t<T>, alia_box*>;

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
concept ValidLayoutPack
    = sizeof...(Args) > 0
   && layout_args_contain_only_valid_types_and_one_content_block<Args...>&&
          layout_args_must_have_maximum_one_flag_set<Args...>&&
              layout_args_must_have_maximum_one_box<Args...>;

template<class Begin, class End, class... Args>
    requires ValidLayoutPack<Args...>
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

template<class... ArgPack>
    requires impl::ValidLayoutPack<ArgPack...>
void
row(context& ctx, ArgPack&&... args)
{
    impl::simple_layout_container(
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
    impl::simple_layout_container(
        ctx,
        alia_layout_column_begin,
        alia_layout_column_end,
        std::forward<ArgPack>(args)...);
}

template<class... ArgPack>
void
flow(context& ctx, ArgPack&&... args)
{
    impl::simple_layout_container(
        ctx,
        alia_layout_flow_begin,
        alia_layout_flow_end,
        std::forward<ArgPack>(args)...);
}

template<class... ArgPack>
void
block_flow(context& ctx, ArgPack&&... args)
{
    impl::simple_layout_container(
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
inset(context& ctx, alia_insets insets, Content&& content)
{
    alia_layout_inset_begin(&ctx, insets, 0);
    std::forward<Content>(content)();
    alia_layout_inset_end(&ctx);
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

} // namespace alia
