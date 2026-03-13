#pragma once

#include <tuple>
#include <type_traits>

template<class T>
struct is_layout_modifier : std::false_type
{
};

template<class T>
concept LayoutModifier = is_layout_modifier<T>::value;

template<class T>
concept LayoutModifierPack
    = requires(T t) { typename std::remove_cvref_t<T>::values; };

template<class... Ts>
struct layout_mods_t
{
    std::tuple<Ts...> values;
};

// Modifier | Modifier
template<LayoutModifier A, LayoutModifier B>
constexpr auto
operator|(A a, B b)
{
    return layout_mods_t<A, B>{std::tuple{a, b}};
}

// Modifier | Pack
template<LayoutModifier A, class... Bs>
constexpr auto
operator|(A a, layout_mods_t<Bs...> b)
{
    return layout_mods_t<A, Bs...>{std::tuple_cat(std::tuple{a}, b.values)};
}

// Pack | Modifier
template<class... As, LayoutModifier B>
constexpr auto
operator|(layout_mods_t<As...> a, B b)
{
    return layout_mods_t<As..., B>{std::tuple_cat(a.values, std::tuple{b})};
}

// Pack | Pack
template<class... As, class... Bs>
constexpr auto
operator|(layout_mods_t<As...> a, layout_mods_t<Bs...> b)
{
    return layout_mods_t<As..., Bs...>{std::tuple_cat(a.values, b.values)};
}

template<class Context, class Content>
void
apply_mods(Context& ctx, layout_mods_t<>, Content&& content)
{
    std::forward<Content>(content)();
}

template<class Context, class Content, LayoutModifier Mod, class... Rest>
void
apply_mods(Context& ctx, layout_mods_t<Mod, Rest...> mods, Content&& content)
{
    apply_mod(ctx, std::get<Mod>(mods.values), [&] {
        apply_mods(
            ctx,
            layout_mods_t<Rest...>{
                std::tuple<Rest...>{std::get<Rest>(mods.values)...}},
            std::forward<Content>(content));
    });
}

struct align_right_t
{
};
template<>
struct is_layout_modifier<align_right_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, align_right_t, Content&& content)
{
    alignment_override(ctx, ALIGN_RIGHT, std::forward<Content>(content));
}

struct fill_x_t
{
};
template<>
struct is_layout_modifier<fill_x_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, fill_x_t, Content&& content)
{
    alignment_override(ctx, FILL_X, std::forward<Content>(content));
}

struct center_y_t
{
};
template<>
struct is_layout_modifier<center_y_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, center_y_t, Content&& content)
{
    alignment_override(ctx, CENTER_Y, std::forward<Content>(content));
}

struct min_width_t
{
    float width;
};
template<>
struct is_layout_modifier<min_width_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, min_width_t m, Content&& content)
{
    min_size_constraint(
        ctx, {.x = m.width, .y = 0}, std::forward<Content>(content));
}

struct min_height_t
{
    float height;
};
template<>
struct is_layout_modifier<min_height_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, min_height_t m, Content&& content)
{
    min_size_constraint(
        ctx, {.x = 0, .y = m.height}, std::forward<Content>(content));
}

struct min_size_t
{
    alia_vec2f size;
};
template<>
struct is_layout_modifier<min_size_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, min_size_t m, Content&& content)
{
    min_size_constraint(ctx, m.size, std::forward<Content>(content));
}

struct margins_t
{
    alia_insets insets;
};
template<>
struct is_layout_modifier<margins_t> : std::true_type
{
};

template<class Context, class Content>
void
apply_mod(Context& ctx, margins_t m, Content&& content)
{
    inset(ctx, m.insets, std::forward<Content>(content));
}

constexpr align_right_t align_right;
constexpr fill_x_t fill_x;
constexpr center_y_t center_y;

constexpr min_width_t
min_width(float w)
{
    return {w};
}
constexpr min_height_t
min_height(float h)
{
    return {h};
}
constexpr min_size_t
min_size(alia_vec2f size)
{
    return {size};
}
constexpr margins_t
margins(alia_insets insets)
{
    return {insets};
}
