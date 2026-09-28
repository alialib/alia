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

// minimum length along the parent main axis, in logical pixels
struct length_spec
{
    float value;
};

// minimum breadth along the parent cross axis, in logical pixels
struct breadth_spec
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

// Specify a minimum length along the parent main axis, in logical pixels.
inline length_spec
length(float value)
{
    return length_spec{value};
}

// Specify a minimum breadth along the parent cross axis, in logical pixels.
inline breadth_spec
breadth(float value)
{
    return breadth_spec{value};
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

    // Allow construction from any layout_like value (e.g. `pad(8)`, `GROW |
    // pad(8)`) so type-erased `layout_options` parameters accept the same
    // packs as templated `layout_like` ones.
    template<class T>
    layout_options(T const& t)
        requires(
            !std::same_as<std::decay_t<T>, layout_options> && requires(
                T const& u) {
                { as_layout_options(u) } -> std::same_as<layout_options>;
            })
        : layout_options(as_layout_options(t))
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

    bool
    has_size() const
    {
        return has_width() || has_height();
    }
};

// empty layout options
inline layout_options const default_layout{};

namespace detail {

template<class T>
constexpr bool is_empty_layout_piece_v
    = std::is_same_v<std::decay_t<T>, empty_layout_piece>;

template<class T>
constexpr bool is_width_spec_v = std::is_same_v<std::decay_t<T>, width_spec>;

template<class T>
constexpr bool is_height_spec_v = std::is_same_v<std::decay_t<T>, height_spec>;

template<class T>
constexpr bool is_length_spec_v = std::is_same_v<std::decay_t<T>, length_spec>;

template<class T>
constexpr bool is_breadth_spec_v
    = std::is_same_v<std::decay_t<T>, breadth_spec>;

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
        "layout pack already specifies width() or length()");
    static_assert(
        is_empty_layout_piece_v<Height> || is_height_spec_v<Height>,
        "cannot combine width() with breadth()");
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
        "layout pack already specifies height() or breadth()");
    static_assert(
        is_empty_layout_piece_v<Width> || is_width_spec_v<Width>,
        "cannot combine height() with length()");
    return layout_spec<Flags, Pad, Width, height_spec, Growth>{
        spec.flags, spec.pad, spec.width, height, spec.growth};
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
with_length(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, length_spec length)
{
    static_assert(
        is_empty_layout_piece_v<Width>,
        "layout pack already specifies length() or width()");
    static_assert(
        is_empty_layout_piece_v<Height> || is_breadth_spec_v<Height>,
        "cannot combine length() with height()");
    if constexpr (is_empty_layout_piece_v<Flags>)
    {
        return layout_spec<layout_flag_set, Pad, length_spec, Height, Growth>{
            AXIS_RELATIVE_SIZE, spec.pad, length, spec.height, spec.growth};
    }
    else
    {
        return layout_spec<Flags, Pad, length_spec, Height, Growth>{
            spec.flags | AXIS_RELATIVE_SIZE,
            spec.pad,
            length,
            spec.height,
            spec.growth};
    }
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
with_breadth(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, breadth_spec breadth)
{
    static_assert(
        is_empty_layout_piece_v<Height>,
        "layout pack already specifies breadth() or height()");
    static_assert(
        is_empty_layout_piece_v<Width> || is_length_spec_v<Width>,
        "cannot combine breadth() with width()");
    if constexpr (is_empty_layout_piece_v<Flags>)
    {
        return layout_spec<layout_flag_set, Pad, Width, breadth_spec, Growth>{
            AXIS_RELATIVE_SIZE, spec.pad, spec.width, breadth, spec.growth};
    }
    else
    {
        return layout_spec<Flags, Pad, Width, breadth_spec, Growth>{
            spec.flags | AXIS_RELATIVE_SIZE,
            spec.pad,
            spec.width,
            breadth,
            spec.growth};
    }
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

// Convert a main-axis length to erased layout options.
inline layout_options
as_layout_options(length_spec const& l)
{
    layout_options layout;
    layout.flags = AXIS_RELATIVE_SIZE;
    layout.present = layout_options::width_bit;
    layout.width_value = l.value;
    return layout;
}

// Convert a cross-axis breadth to erased layout options.
inline layout_options
as_layout_options(breadth_spec const& b)
{
    layout_options layout;
    layout.flags = AXIS_RELATIVE_SIZE;
    layout.present = layout_options::height_bit;
    layout.height_value = b.value;
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
    if constexpr (
        detail::is_width_spec_v<Width> || detail::is_length_spec_v<Width>)
    {
        out.present |= layout_options::width_bit;
        out.width_value = spec.width.value;
    }
    if constexpr (
        detail::is_height_spec_v<Height> || detail::is_breadth_spec_v<Height>)
    {
        out.present |= layout_options::height_bit;
        out.height_value = spec.height.value;
    }
    if constexpr (
        detail::is_length_spec_v<Width> || detail::is_breadth_spec_v<Height>)
    {
        out.flags = out.flags | AXIS_RELATIVE_SIZE;
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
    layout.flags = layout.flags & ~AXIS_RELATIVE_SIZE;
    layout.present |= layout_options::width_bit;
    layout.width_value = w.value;
    return layout;
}

inline layout_options&
operator|=(layout_options& layout, height_spec const& h)
{
    layout.flags = layout.flags & ~AXIS_RELATIVE_SIZE;
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
operator|=(layout_options& layout, length_spec const& l)
{
    layout.flags = layout.flags | AXIS_RELATIVE_SIZE;
    layout.present |= layout_options::width_bit;
    layout.width_value = l.value;
    return layout;
}

inline layout_options&
operator|=(layout_options& layout, breadth_spec const& b)
{
    layout.flags = layout.flags | AXIS_RELATIVE_SIZE;
    layout.present |= layout_options::height_bit;
    layout.height_value = b.value;
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
operator|(layout_options layout, length_spec const& l)
{
    return layout |= l;
}

inline layout_options
operator|(layout_options layout, breadth_spec const& b)
{
    return layout |= b;
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

inline layout_options
operator|(length_spec const& l, layout_options const& layout)
{
    return as_layout_options(l) | layout;
}

inline layout_options
operator|(breadth_spec const& b, layout_options const& layout)
{
    return as_layout_options(b) | layout;
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

inline layout_spec<layout_flag_set, empty_layout_piece, length_spec>
operator|(layout_flag_set flags, length_spec const& l)
{
    return {flags | AXIS_RELATIVE_SIZE, {}, l, {}, {}};
}

inline layout_spec<layout_flag_set, empty_layout_piece, length_spec>
operator|(length_spec const& l, layout_flag_set flags)
{
    return {flags | AXIS_RELATIVE_SIZE, {}, l, {}, {}};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    empty_layout_piece,
    breadth_spec>
operator|(layout_flag_set flags, breadth_spec const& b)
{
    return {flags | AXIS_RELATIVE_SIZE, {}, {}, b, {}};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    empty_layout_piece,
    breadth_spec>
operator|(breadth_spec const& b, layout_flag_set flags)
{
    return {flags | AXIS_RELATIVE_SIZE, {}, {}, b, {}};
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

inline layout_spec<layout_flag_set, pad_spec, length_spec>
operator|(pad_spec const& p, length_spec const& l)
{
    return {AXIS_RELATIVE_SIZE, p, l, {}, {}};
}

inline layout_spec<layout_flag_set, pad_spec, length_spec>
operator|(length_spec const& l, pad_spec const& p)
{
    return {AXIS_RELATIVE_SIZE, p, l, {}, {}};
}

inline layout_spec<layout_flag_set, pad_spec, empty_layout_piece, breadth_spec>
operator|(pad_spec const& p, breadth_spec const& b)
{
    return {AXIS_RELATIVE_SIZE, p, {}, b, {}};
}

inline layout_spec<layout_flag_set, pad_spec, empty_layout_piece, breadth_spec>
operator|(breadth_spec const& b, pad_spec const& p)
{
    return {AXIS_RELATIVE_SIZE, p, {}, b, {}};
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

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    length_spec,
    breadth_spec>
operator|(length_spec const& l, breadth_spec const& b)
{
    return {AXIS_RELATIVE_SIZE, {}, l, b, {}};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    length_spec,
    breadth_spec>
operator|(breadth_spec const& b, length_spec const& l)
{
    return {AXIS_RELATIVE_SIZE, {}, l, b, {}};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    length_spec,
    empty_layout_piece,
    growth_spec>
operator|(length_spec const& l, growth_spec const& g)
{
    return {AXIS_RELATIVE_SIZE, {}, l, {}, g};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    length_spec,
    empty_layout_piece,
    growth_spec>
operator|(growth_spec const& g, length_spec const& l)
{
    return {AXIS_RELATIVE_SIZE, {}, l, {}, g};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    empty_layout_piece,
    breadth_spec,
    growth_spec>
operator|(breadth_spec const& b, growth_spec const& g)
{
    return {AXIS_RELATIVE_SIZE, {}, {}, b, g};
}

inline layout_spec<
    layout_flag_set,
    empty_layout_piece,
    empty_layout_piece,
    breadth_spec,
    growth_spec>
operator|(growth_spec const& g, breadth_spec const& b)
{
    return {AXIS_RELATIVE_SIZE, {}, {}, b, g};
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

length_spec
operator|(length_spec, length_spec) = delete;

breadth_spec
operator|(breadth_spec, breadth_spec) = delete;

// Absolute and axis-relative size bags are exclusive.
width_spec
operator|(width_spec, length_spec) = delete;
length_spec
operator|(length_spec, width_spec) = delete;
height_spec
operator|(height_spec, breadth_spec) = delete;
breadth_spec
operator|(breadth_spec, height_spec) = delete;
width_spec
operator|(width_spec, breadth_spec) = delete;
breadth_spec
operator|(breadth_spec, width_spec) = delete;
height_spec
operator|(height_spec, length_spec) = delete;
length_spec
operator|(length_spec, height_spec) = delete;

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
    layout_spec<Flags, Pad, Width, Height, Growth> spec, length_spec const& l)
{
    return detail::with_length(spec, l);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    length_spec const& l, layout_spec<Flags, Pad, Width, Height, Growth> spec)
{
    return detail::with_length(spec, l);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    layout_spec<Flags, Pad, Width, Height, Growth> spec, breadth_spec const& b)
{
    return detail::with_breadth(spec, b);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
operator|(
    breadth_spec const& b, layout_spec<Flags, Pad, Width, Height, Growth> spec)
{
    return detail::with_breadth(spec, b);
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

// Content size from width/height slots (missing axes are 0).
inline alia_vec2f
layout_content_size(layout_options const& layout)
{
    return alia_vec2f_make(
        layout.has_width() ? layout.width_value : 0.f,
        layout.has_height() ? layout.height_value : 0.f);
}

inline alia_vec2f
layout_content_size(layout_flag_set)
{
    return alia_vec2f_make(0.f, 0.f);
}

inline alia_vec2f
layout_content_size(null_flag_set)
{
    return alia_vec2f_make(0.f, 0.f);
}

inline alia_vec2f
layout_content_size(pad_spec const&)
{
    return alia_vec2f_make(0.f, 0.f);
}

inline alia_vec2f
layout_content_size(growth_spec const&)
{
    return alia_vec2f_make(0.f, 0.f);
}

inline alia_vec2f
layout_content_size(width_spec const& w)
{
    return alia_vec2f_make(w.value, 0.f);
}

inline alia_vec2f
layout_content_size(height_spec const& h)
{
    return alia_vec2f_make(0.f, h.value);
}

inline alia_vec2f
layout_content_size(length_spec const& l)
{
    return alia_vec2f_make(l.value, 0.f);
}

inline alia_vec2f
layout_content_size(breadth_spec const& b)
{
    return alia_vec2f_make(0.f, b.value);
}

template<class Flags, class Pad, class Width, class Height, class Growth>
alia_vec2f
layout_content_size(layout_spec<Flags, Pad, Width, Height, Growth> const& spec)
{
    float w = 0.f;
    float h = 0.f;
    if constexpr (
        detail::is_width_spec_v<Width> || detail::is_length_spec_v<Width>)
    {
        w = spec.width.value;
    }
    if constexpr (
        detail::is_height_spec_v<Height> || detail::is_breadth_spec_v<Height>)
    {
        h = spec.height.value;
    }
    return alia_vec2f_make(w, h);
}

// Clear width/height so `apply_layout` will not open a min-size wrapper.
// Size-less pieces are returned unchanged so typed `apply_layout` overloads
// stay selected.
inline layout_options
without_size(layout_options layout)
{
    layout.present = static_cast<std::uint8_t>(
        layout.present
        & ~(layout_options::width_bit | layout_options::height_bit));
    layout.width_value = 0.f;
    layout.height_value = 0.f;
    return layout;
}

inline layout_flag_set
without_size(layout_flag_set flags)
{
    return flags;
}

inline null_flag_set
without_size(null_flag_set flags)
{
    return flags;
}

inline pad_spec
without_size(pad_spec p)
{
    return p;
}

inline growth_spec
without_size(growth_spec g)
{
    return g;
}

// A lone size piece has nothing left once size is cleared.
inline layout_flag_set
without_size(width_spec)
{
    return NO_FLAGS;
}

inline layout_flag_set
without_size(height_spec)
{
    return NO_FLAGS;
}

// Relative size pieces still need AXIS_RELATIVE_SIZE on the leaf.
inline layout_flag_set
without_size(length_spec)
{
    return AXIS_RELATIVE_SIZE;
}

inline layout_flag_set
without_size(breadth_spec)
{
    return AXIS_RELATIVE_SIZE;
}

template<class Flags, class Pad, class Width, class Height, class Growth>
auto
without_size(layout_spec<Flags, Pad, Width, Height, Growth> spec)
{
    return layout_spec<
        Flags,
        Pad,
        empty_layout_piece,
        empty_layout_piece,
        Growth>{spec.flags, spec.pad, {}, {}, spec.growth};
}

// Adjust `flags` to work for a leaf that is flush by default.
inline layout_flag_set
add_default_flush(layout_flag_set flags)
{
    if ((flags & SPACING_MASK) == layout_flag_set(NO_FLAGS))
        return flags | FLUSH;
    return flags;
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

// Axis-relative sizes are recorded on the leaf; they are not absolute
// wrappers.
template<class Fn>
void
apply_layout(context& ctx, length_spec const&, Fn&& fn)
{
    (void) ctx;
    std::forward<Fn>(fn)(AXIS_RELATIVE_SIZE);
}

template<class Fn>
void
apply_layout(context& ctx, breadth_spec const&, Fn&& fn)
{
    (void) ctx;
    std::forward<Fn>(fn)(AXIS_RELATIVE_SIZE);
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
    if constexpr (
        detail::is_length_spec_v<Width> || detail::is_breadth_spec_v<Height>)
    {
        flags = flags | AXIS_RELATIVE_SIZE;
    }

    constexpr bool has_absolute_width = detail::is_width_spec_v<Width>;
    constexpr bool has_absolute_height = detail::is_height_spec_v<Height>;
    constexpr bool has_min_size = has_absolute_width || has_absolute_height;

    if constexpr (!detail::is_empty_layout_piece_v<Pad>)
        alia_layout_edge_offsets_begin(&ctx, spec.pad.offsets, 0);
    if constexpr (!detail::is_empty_layout_piece_v<Growth>)
        alia_layout_growth_override_begin(&ctx, spec.growth.value);
    if constexpr (has_min_size)
    {
        float const w = has_absolute_width ? spec.width.value : 0.f;
        float const h = has_absolute_height ? spec.height.value : 0.f;
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
    bool const absolute_size
        = (layout.has_width() || layout.has_height())
       && (raw_code(layout.flags) & raw_code(AXIS_RELATIVE_SIZE)) == 0;
    if (absolute_size)
    {
        alia_layout_min_size_begin(
            &ctx,
            detail::min_size_vec(
                layout.has_width() ? layout.width_value : 0.f,
                layout.has_height() ? layout.height_value : 0.f));
    }

    std::forward<Fn>(fn)(layout.flags);

    if (absolute_size)
        alia_layout_min_size_end(&ctx);
    if (layout.has_growth())
        alia_layout_growth_override_end(&ctx);
    if (layout.has_pad())
        alia_layout_edge_offsets_end(&ctx);
}

} // namespace alia
