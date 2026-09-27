#pragma once

#include <alia/abi/base/geometry/edge_offsets.h>
#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/context.h>
#include <alia/ui/layout/flags.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>

// This file defines the C++ layout parameter strategy. It is designed so that
// all layout-related options are combined into a single function parameter.
// The function can declare this parameter as either a templated pack of
// options or a single "type-erased" `layout_options` object. In either case,
// the caller passes layout options as a single composition, e.g.:
// `GROW | pad(8)`

namespace alia {

// sentinel for an absent slot in a typed layout pack
struct empty_layout_piece
{
};

// padding around a control, in logical pixels
struct pad_spec
{
    alia_edge_offsets offsets;
};

// minimum width constraint, in logical pixels
struct width_spec
{
    float value;
};

// minimum height constraint, in logical pixels
struct height_spec
{
    float value;
};

// growth factor override
struct growth_spec
{
    float value;
};

// Specify uniform padding around a control, in logical pixels.
inline pad_spec
pad(float uniform)
{
    return pad_spec{alia_edge_offsets_make_uniform(uniform)};
}

// Specify non-uniform padding around a control, in logical pixels.
inline pad_spec
pad(alia_edge_offsets offsets)
{
    return pad_spec{offsets};
}

// Specify a minimum width constraint, in logical pixels.
inline width_spec
width(float w)
{
    return width_spec{w};
}

// Specify a minimum height constraint, in logical pixels.
inline height_spec
height(float h)
{
    return height_spec{h};
}

// Specify a growth factor override.
inline growth_spec
growth(float value)
{
    return growth_spec{value};
}

// typed layout pack - Absent slots use `empty_layout_piece` so apply can drop
// those wrappers at compile time.
template<
    class Flags = empty_layout_piece,
    class Pad = empty_layout_piece,
    class Width = empty_layout_piece,
    class Height = empty_layout_piece,
    class Growth = empty_layout_piece>
struct layout_spec
{
    Flags flags{};
    Pad pad{};
    Width width{};
    Height height{};
    Growth growth{};
};

// type-erased layout options for a leaf or leaf-like control
struct layout_options
{
    // which optional fields are set
    enum present_bits : std::uint8_t
    {
        pad_bit = 1 << 0,
        width_bit = 1 << 1,
        height_bit = 1 << 2,
        growth_bit = 1 << 3,
    };

    // layout flags
    layout_flag_set flags = NO_FLAGS;
    // presence mask for optional fields
    std::uint8_t present = 0;
    // padding applied via an edge-offsets wrapper
    alia_edge_offsets pad_offsets{};
    // minimum width constraint, in logical pixels
    float width_value = 0.f;
    // minimum height constraint, in logical pixels
    float height_value = 0.f;
    // growth factor override
    float growth_value = 0.f;

    layout_options() = default;

    layout_options(null_flag_set) : flags(NO_FLAGS)
    {
    }

    layout_options(layout_flag_set flags) : flags(flags)
    {
    }

    bool
    has_pad() const
    {
        return (present & pad_bit) != 0;
    }

    bool
    has_width() const
    {
        return (present & width_bit) != 0;
    }

    bool
    has_height() const
    {
        return (present & height_bit) != 0;
    }

    bool
    has_growth() const
    {
        return (present & growth_bit) != 0;
    }
};

// empty layout options
inline layout_options const default_layout{};

namespace detail {

template<class T>
constexpr bool is_empty_layout_piece_v
    = std::is_same_v<std::decay_t<T>, empty_layout_piece>;

inline pad_spec
add_pad(pad_spec a, pad_spec const& b)
{
    a.offsets = alia_edge_offsets_add(a.offsets, b.offsets);
    return a;
}

inline alia_vec2f
min_size_vec(float width, float height)
{
    return alia_vec2f_make(width, height);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
with_flags(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, layout_flag_set flags)
{
    if constexpr (is_empty_layout_piece_v<Flags>)
    {
        return layout_spec<layout_flag_set, Pad, Width, Height, Growth>{
            flags, spec.pad, spec.width, spec.height, spec.growth};
    }
    else
    {
        spec.flags = spec.flags | flags;
        return spec;
    }
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
with_pad(layout_spec<Flags, Pad, Width, Height, Growth> spec, pad_spec pad)
{
    if constexpr (is_empty_layout_piece_v<Pad>)
    {
        return layout_spec<Flags, pad_spec, Width, Height, Growth>{
            spec.flags, pad, spec.width, spec.height, spec.growth};
    }
    else
    {
        spec.pad = add_pad(spec.pad, pad);
        return spec;
    }
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
with_width(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, width_spec width)
{
    static_assert(
        is_empty_layout_piece_v<Width>,
        "layout pack already specifies width()");
    return layout_spec<Flags, Pad, width_spec, Height, Growth>{
        spec.flags, spec.pad, width, spec.height, spec.growth};
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
with_height(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, height_spec height)
{
    static_assert(
        is_empty_layout_piece_v<Height>,
        "layout pack already specifies height()");
    return layout_spec<Flags, Pad, Width, height_spec, Growth>{
        spec.flags, spec.pad, spec.width, height, spec.growth};
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
with_growth(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, growth_spec growth)
{
    static_assert(
        is_empty_layout_piece_v<Growth>,
        "layout pack already specifies growth()");
    return layout_spec<Flags, Pad, Width, Height, growth_spec>{
        spec.flags, spec.pad, spec.width, spec.height, growth};
}

} // namespace detail

// Convert `layout` to erased layout options.
inline layout_options
as_layout_options(layout_options const& layout)
{
    return layout;
}

// Convert layout flags to erased layout options.
inline layout_options
as_layout_options(layout_flag_set flags)
{
    return layout_options{flags};
}

// Convert `NO_FLAGS` to erased layout options.
inline layout_options
as_layout_options(null_flag_set)
{
    return layout_options{};
}

// Convert padding to erased layout options.
inline layout_options
as_layout_options(pad_spec const& p)
{
    layout_options layout;
    layout.present = layout_options::pad_bit;
    layout.pad_offsets = p.offsets;
    return layout;
}

// Convert a width constraint to erased layout options.
inline layout_options
as_layout_options(width_spec const& w)
{
    layout_options layout;
    layout.present = layout_options::width_bit;
    layout.width_value = w.value;
    return layout;
}

// Convert a height constraint to erased layout options.
inline layout_options
as_layout_options(height_spec const& h)
{
    layout_options layout;
    layout.present = layout_options::height_bit;
    layout.height_value = h.value;
    return layout;
}

// Convert a growth override to erased layout options.
inline layout_options
as_layout_options(growth_spec const& g)
{
    layout_options layout;
    layout.present = layout_options::growth_bit;
    layout.growth_value = g.value;
    return layout;
}

// Convert a typed layout pack to erased layout options.
template<class Flags, class Pad, class Width, class Height, class Growth>
layout_options
as_layout_options(layout_spec<Flags, Pad, Width, Height, Growth> const& spec)
{
    layout_options out;
    if constexpr (!detail::is_empty_layout_piece_v<Flags>)
        out.flags = spec.flags;
    if constexpr (!detail::is_empty_layout_piece_v<Pad>)
    {
        out.present |= layout_options::pad_bit;
        out.pad_offsets = spec.pad.offsets;
    }
    if constexpr (!detail::is_empty_layout_piece_v<Width>)
    {
        out.present |= layout_options::width_bit;
        out.width_value = spec.width.value;
    }
    if constexpr (!detail::is_empty_layout_piece_v<Height>)
    {
        out.present |= layout_options::height_bit;
        out.height_value = spec.height.value;
    }
    if constexpr (!detail::is_empty_layout_piece_v<Growth>)
    {
        out.present |= layout_options::growth_bit;
        out.growth_value = spec.growth.value;
    }
    return out;
}

// concept for types that can be converted to `layout_options`
template<class T>
concept layout_like = requires(T const& t) {
    { as_layout_options(t) } -> std::same_as<layout_options>;
};

inline layout_options&
operator|=(layout_options& layout, layout_flag_set flags)
{
    layout.flags = layout.flags | flags;
    return layout;
}

inline layout_options&
operator|=(layout_options& layout, pad_spec const& p)
{
    if (layout.has_pad())
        layout.pad_offsets
            = alia_edge_offsets_add(layout.pad_offsets, p.offsets);
    else
    {
        layout.present |= layout_options::pad_bit;
        layout.pad_offsets = p.offsets;
    }
    return layout;
}

inline layout_options&
operator|=(layout_options& layout, width_spec const& w)
{
    layout.present |= layout_options::width_bit;
    layout.width_value = w.value;
    return layout;
}

inline layout_options&
operator|=(layout_options& layout, height_spec const& h)
{
    layout.present |= layout_options::height_bit;
    layout.height_value = h.value;
    return layout;
}

inline layout_options&
operator|=(layout_options& layout, growth_spec const& g)
{
    layout.present |= layout_options::growth_bit;
    layout.growth_value = g.value;
    return layout;
}

inline layout_options&
operator|=(layout_options& layout, layout_options const& other)
{
    layout.flags = layout.flags | other.flags;
    if (other.has_pad())
    {
        if (layout.has_pad())
            layout.pad_offsets
                = alia_edge_offsets_add(layout.pad_offsets, other.pad_offsets);
        else
        {
            layout.present |= layout_options::pad_bit;
            layout.pad_offsets = other.pad_offsets;
        }
    }
    if (other.has_width())
    {
        layout.present |= layout_options::width_bit;
        layout.width_value = other.width_value;
    }
    if (other.has_height())
    {
        layout.present |= layout_options::height_bit;
        layout.height_value = other.height_value;
    }
    if (other.has_growth())
    {
        layout.present |= layout_options::growth_bit;
        layout.growth_value = other.growth_value;
    }
    return layout;
}

inline layout_options
operator|(layout_options layout, layout_flag_set flags)
{
    return layout |= flags;
}

inline layout_options
operator|(layout_options layout, pad_spec const& p)
{
    return layout |= p;
}

inline layout_options
operator|(layout_options layout, width_spec const& w)
{
    return layout |= w;
}

inline layout_options
operator|(layout_options layout, height_spec const& h)
{
    return layout |= h;
}

inline layout_options
operator|(layout_options layout, growth_spec const& g)
{
    return layout |= g;
}

inline layout_options
operator|(layout_options layout, layout_options const& other)
{
    return layout |= other;
}

inline layout_options
operator|(layout_flag_set flags, layout_options const& layout)
{
    return as_layout_options(flags) | layout;
}

inline layout_options
operator|(pad_spec const& p, layout_options const& layout)
{
    return as_layout_options(p) | layout;
}

inline layout_options
operator|(width_spec const& w, layout_options const& layout)
{
    return as_layout_options(w) | layout;
}

inline layout_options
operator|(height_spec const& h, layout_options const& layout)
{
    return as_layout_options(h) | layout;
}

inline layout_options
operator|(growth_spec const& g, layout_options const& layout)
{
    return as_layout_options(g) | layout;
}

inline layout_spec<layout_flag_set, pad_spec>
operator|(layout_flag_set flags, pad_spec const& p)
{
    return {flags, p, {}, {}, {}};
}

inline layout_spec<layout_flag_set, pad_spec>
operator|(pad_spec const& p, layout_flag_set flags)
{
    return {flags, p, {}, {}, {}};
}

inline layout_spec<layout_flag_set, pad_spec>
operator|(null_flag_set, pad_spec const& p)
{
    return {NO_FLAGS, p, {}, {}, {}};
}

inline layout_spec<layout_flag_set, pad_spec>
operator|(pad_spec const& p, null_flag_set)
{
    return {NO_FLAGS, p, {}, {}, {}};
}

inline layout_spec<layout_flag_set, empty_layout_piece, width_spec>
operator|(layout_flag_set flags, width_spec const& w)
{
    return {flags, {}, w, {}, {}};
}

inline layout_spec<layout_flag_set, empty_layout_piece, width_spec>
operator|(width_spec const& w, layout_flag_set flags)
{
    return {flags, {}, w, {}, {}};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    empty_layout_piece,
    height_spec>
operator|(layout_flag_set flags, height_spec const& h)
{
    return {flags, {}, {}, h, {}};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    empty_layout_piece,
    height_spec>
operator|(height_spec const& h, layout_flag_set flags)
{
    return {flags, {}, {}, h, {}};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    empty_layout_piece,
    empty_layout_piece,
    growth_spec>
operator|(layout_flag_set flags, growth_spec const& g)
{
    return {flags, {}, {}, {}, g};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    empty_layout_piece,
    empty_layout_piece,
    growth_spec>
operator|(growth_spec const& g, layout_flag_set flags)
{
    return {flags, {}, {}, {}, g};
}

inline layout_spec<empty_layout_piece, pad_spec, width_spec>
operator|(pad_spec const& p, width_spec const& w)
{
    return {{}, p, w, {}, {}};
}

inline layout_spec<empty_layout_piece, pad_spec, width_spec>
operator|(width_spec const& w, pad_spec const& p)
{
    return {{}, p, w, {}, {}};
}

inline layout_spec<
    empty_layout_piece,
    pad_spec,
    empty_layout_piece,
    height_spec>
operator|(pad_spec const& p, height_spec const& h)
{
    return {{}, p, {}, h, {}};
}

inline layout_spec<
    empty_layout_piece,
    pad_spec,
    empty_layout_piece,
    height_spec>
operator|(height_spec const& h, pad_spec const& p)
{
    return {{}, p, {}, h, {}};
}

inline layout_spec<
    empty_layout_piece,
    pad_spec,
    empty_layout_piece,
    empty_layout_piece,
    growth_spec>
operator|(pad_spec const& p, growth_spec const& g)
{
    return {{}, p, {}, {}, g};
}

inline layout_spec<
    empty_layout_piece,
    pad_spec,
    empty_layout_piece,
    empty_layout_piece,
    growth_spec>
operator|(growth_spec const& g, pad_spec const& p)
{
    return {{}, p, {}, {}, g};
}

inline layout_spec<
    empty_layout_piece,
    empty_layout_piece,
    width_spec,
    height_spec>
operator|(width_spec const& w, height_spec const& h)
{
    return {{}, {}, w, h, {}};
}

inline layout_spec<
    empty_layout_piece,
    empty_layout_piece,
    width_spec,
    height_spec>
operator|(height_spec const& h, width_spec const& w)
{
    return {{}, {}, w, h, {}};
}

inline layout_spec<
    empty_layout_piece,
    empty_layout_piece,
    width_spec,
    empty_layout_piece,
    growth_spec>
operator|(width_spec const& w, growth_spec const& g)
{
    return {{}, {}, w, {}, g};
}

inline layout_spec<
    empty_layout_piece,
    empty_layout_piece,
    width_spec,
    empty_layout_piece,
    growth_spec>
operator|(growth_spec const& g, width_spec const& w)
{
    return {{}, {}, w, {}, g};
}

inline layout_spec<
    empty_layout_piece,
    empty_layout_piece,
    empty_layout_piece,
    height_spec,
    growth_spec>
operator|(height_spec const& h, growth_spec const& g)
{
    return {{}, {}, {}, h, g};
}

inline layout_spec<
    empty_layout_piece,
    empty_layout_piece,
    empty_layout_piece,
    height_spec,
    growth_spec>
operator|(growth_spec const& g, height_spec const& h)
{
    return {{}, {}, {}, h, g};
}

inline pad_spec
operator|(pad_spec a, pad_spec const& b)
{
    return detail::add_pad(a, b);
}

width_spec
operator|(width_spec, width_spec) = delete;

height_spec
operator|(height_spec, height_spec) = delete;

growth_spec
operator|(growth_spec, growth_spec) = delete;

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, layout_flag_set flags)
{
    return detail::with_flags(spec, flags);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    layout_flag_set flags, layout_spec<Flags, Pad, Width, Height, Growth> spec)
{
    return detail::with_flags(spec, flags);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, pad_spec const& p)
{
    return detail::with_pad(spec, p);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    pad_spec const& p, layout_spec<Flags, Pad, Width, Height, Growth> spec)
{
    return detail::with_pad(spec, p);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, width_spec const& w)
{
    return detail::with_width(spec, w);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    width_spec const& w, layout_spec<Flags, Pad, Width, Height, Growth> spec)
{
    return detail::with_width(spec, w);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, height_spec const& h)
{
    return detail::with_height(spec, h);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    height_spec const& h, layout_spec<Flags, Pad, Width, Height, Growth> spec)
{
    return detail::with_height(spec, h);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, growth_spec const& g)
{
    return detail::with_growth(spec, g);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    growth_spec const& g, layout_spec<Flags, Pad, Width, Height, Growth> spec)
{
    return detail::with_growth(spec, g);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
layout_options
operator|(
    layout_spec<Flags, Pad, Width, Height, Growth> const& spec,
    layout_options const& layout)
{
    return as_layout_options(spec) | layout;
}

template<class Flags, class Pad, class Width, class Height, class Growth>
layout_options
operator|(
    layout_options const& layout,
    layout_spec<Flags, Pad, Width, Height, Growth> const& spec)
{
    return layout | as_layout_options(spec);
}

// Fill in X alignment only when the caller left that group unset.
inline layout_flag_set
add_default_x_alignment(layout_flag_set flags, layout_flag_set alignment)
{
    if ((flags & X_ALIGNMENT_MASK) == layout_flag_set(NO_FLAGS))
        return flags | alignment;
    return flags;
}

// Fill in Y alignment only when the caller left that group unset.
inline layout_flag_set
add_default_y_alignment(layout_flag_set flags, layout_flag_set alignment)
{
    if ((flags & Y_ALIGNMENT_MASK) == layout_flag_set(NO_FLAGS))
        return flags | alignment;
    return flags;
}

// Fill in X and Y alignment only where the caller left those groups unset.
inline layout_flag_set
add_default_alignment(
    layout_flag_set flags,
    layout_flag_set x_alignment,
    layout_flag_set y_alignment)
{
    return add_default_y_alignment(
        add_default_x_alignment(flags, x_alignment), y_alignment);
}

inline layout_options
add_default_x_alignment(layout_options layout, layout_flag_set alignment)
{
    layout.flags = add_default_x_alignment(layout.flags, alignment);
    return layout;
}

inline layout_options
add_default_y_alignment(layout_options layout, layout_flag_set alignment)
{
    layout.flags = add_default_y_alignment(layout.flags, alignment);
    return layout;
}

inline layout_options
add_default_alignment(
    layout_options layout,
    layout_flag_set x_alignment,
    layout_flag_set y_alignment)
{
    layout.flags
        = add_default_alignment(layout.flags, x_alignment, y_alignment);
    return layout;
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
add_default_x_alignment(
    layout_spec<Flags, Pad, Width, Height, Growth> spec,
    layout_flag_set alignment)
{
    if constexpr (detail::is_empty_layout_piece_v<Flags>)
    {
        return layout_spec<layout_flag_set, Pad, Width, Height, Growth>{
            alignment, spec.pad, spec.width, spec.height, spec.growth};
    }
    else
    {
        spec.flags = add_default_x_alignment(spec.flags, alignment);
        return spec;
    }
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
add_default_y_alignment(
    layout_spec<Flags, Pad, Width, Height, Growth> spec,
    layout_flag_set alignment)
{
    if constexpr (detail::is_empty_layout_piece_v<Flags>)
    {
        return layout_spec<layout_flag_set, Pad, Width, Height, Growth>{
            alignment, spec.pad, spec.width, spec.height, spec.growth};
    }
    else
    {
        spec.flags = add_default_y_alignment(spec.flags, alignment);
        return spec;
    }
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
add_default_alignment(
    layout_spec<Flags, Pad, Width, Height, Growth> spec,
    layout_flag_set x_alignment,
    layout_flag_set y_alignment)
{
    return add_default_y_alignment(
        add_default_x_alignment(std::move(spec), x_alignment), y_alignment);
}

// Apply `layout` around `fn(flags)`.
// Wrapper pieces open matching begin/end pairs; absent typed slots are omitted
// entirely at compile time.
template<class Fn>
void
apply_layout(context& ctx, layout_flag_set flags, Fn&& fn)
{
    (void) ctx;
    std::forward<Fn>(fn)(flags);
}

template<class Fn>
void
apply_layout(context& ctx, null_flag_set, Fn&& fn)
{
    (void) ctx;
    std::forward<Fn>(fn)(layout_flag_set{NO_FLAGS});
}

template<class Fn>
void
apply_layout(context& ctx, pad_spec const& p, Fn&& fn)
{
    alia_layout_edge_offsets_begin(&ctx, p.offsets, 0);
    std::forward<Fn>(fn)(layout_flag_set{NO_FLAGS});
    alia_layout_edge_offsets_end(&ctx);
}

template<class Fn>
void
apply_layout(context& ctx, width_spec const& w, Fn&& fn)
{
    alia_layout_min_size_begin(&ctx, detail::min_size_vec(w.value, 0.f));
    std::forward<Fn>(fn)(layout_flag_set{NO_FLAGS});
    alia_layout_min_size_end(&ctx);
}

template<class Fn>
void
apply_layout(context& ctx, height_spec const& h, Fn&& fn)
{
    alia_layout_min_size_begin(&ctx, detail::min_size_vec(0.f, h.value));
    std::forward<Fn>(fn)(layout_flag_set{NO_FLAGS});
    alia_layout_min_size_end(&ctx);
}

template<class Fn>
void
apply_layout(context& ctx, growth_spec const& g, Fn&& fn)
{
    alia_layout_growth_override_begin(&ctx, g.value);
    std::forward<Fn>(fn)(layout_flag_set{NO_FLAGS});
    alia_layout_growth_override_end(&ctx);
}

template<
    class Flags,
    class Pad,
    class Width,
    class Height,
    class Growth,
    class Fn>
void
apply_layout(
    context& ctx,
    layout_spec<Flags, Pad, Width, Height, Growth> const& spec,
    Fn&& fn)
{
    layout_flag_set flags = NO_FLAGS;
    if constexpr (!detail::is_empty_layout_piece_v<Flags>)
        flags = spec.flags;

    constexpr bool has_width = !detail::is_empty_layout_piece_v<Width>;
    constexpr bool has_height = !detail::is_empty_layout_piece_v<Height>;
    constexpr bool has_min_size = has_width || has_height;

    if constexpr (!detail::is_empty_layout_piece_v<Pad>)
        alia_layout_edge_offsets_begin(&ctx, spec.pad.offsets, 0);
    if constexpr (!detail::is_empty_layout_piece_v<Growth>)
        alia_layout_growth_override_begin(&ctx, spec.growth.value);
    if constexpr (has_min_size)
    {
        float const w = has_width ? spec.width.value : 0.f;
        float const h = has_height ? spec.height.value : 0.f;
        alia_layout_min_size_begin(&ctx, detail::min_size_vec(w, h));
    }

    std::forward<Fn>(fn)(flags);

    if constexpr (has_min_size)
        alia_layout_min_size_end(&ctx);
    if constexpr (!detail::is_empty_layout_piece_v<Growth>)
        alia_layout_growth_override_end(&ctx);
    if constexpr (!detail::is_empty_layout_piece_v<Pad>)
        alia_layout_edge_offsets_end(&ctx);
}

template<class Fn>
void
apply_layout(context& ctx, layout_options const& layout, Fn&& fn)
{
    if (layout.has_pad())
        alia_layout_edge_offsets_begin(&ctx, layout.pad_offsets, 0);
    if (layout.has_growth())
        alia_layout_growth_override_begin(&ctx, layout.growth_value);
    if (layout.has_width() || layout.has_height())
    {
        alia_layout_min_size_begin(
            &ctx,
            detail::min_size_vec(
                layout.has_width() ? layout.width_value : 0.f,
                layout.has_height() ? layout.height_value : 0.f));
    }

    std::forward<Fn>(fn)(layout.flags);

    if (layout.has_width() || layout.has_height())
        alia_layout_min_size_end(&ctx);
    if (layout.has_growth())
        alia_layout_growth_override_end(&ctx);
    if (layout.has_pad())
        alia_layout_edge_offsets_end(&ctx);
}

} // namespace alia
