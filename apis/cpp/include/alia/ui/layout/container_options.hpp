#pragma once

#include <alia/abi/ui/geometry.h>
#include <alia/ui/layout/flags.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>

// This file provides the container version of the layout options pattern,
// e.g. `gap(8) | JUSTIFY_CENTER`.

namespace alia {

// gap between children, in logical pixels
struct gap_spec
{
    float value;
};

// gap between flow lines, in logical pixels
struct line_gap_spec
{
    float value;
};

// minimum flow line height, in logical pixels
struct minimum_line_height_spec
{
    float value;
};

// request that the container provide its assigned box for consumption
struct provide_box_spec
{
    alia_box* box;
};

// Specify a gap between children, in logical pixels.
inline gap_spec
gap(float value)
{
    return gap_spec{value};
}

// Specify a gap between flow lines, in logical pixels.
inline line_gap_spec
line_gap(float value)
{
    return line_gap_spec{value};
}

// Specify a minimum flow line height, in logical pixels.
inline minimum_line_height_spec
minimum_line_height(float value)
{
    return minimum_line_height_spec{value};
}

// Request that the container provide its assigned box into `box`.
inline provide_box_spec
provide_box(alia_box& box)
{
    return provide_box_spec{&box};
}

inline provide_box_spec
provide_box(alia_box* box)
{
    return provide_box_spec{box};
}

// type-erased container-policy options
struct container_options
{
    enum present_bits : std::uint8_t
    {
        gap_bit = 1 << 0,
        line_gap_bit = 1 << 1,
        minimum_line_height_bit = 1 << 2,
        provide_box_bit = 1 << 3,
    };

    // container-policy flags
    container_layout_flag_set flags = NO_FLAGS;
    // presence mask for optional fields
    std::uint8_t present = 0;
    // gap between children, in logical pixels
    float gap_value = 0.f;
    // gap between flow lines, in logical pixels
    float line_gap_value = 0.f;
    // minimum flow line height, in logical pixels
    float minimum_line_height_value = 0.f;
    // destination for the container's assigned box
    alia_box* box = nullptr;

    container_options() = default;

    container_options(null_flag_set) : flags(NO_FLAGS)
    {
    }

    container_options(container_layout_flag_set flags) : flags(flags)
    {
    }

    bool
    has_gap() const
    {
        return (present & gap_bit) != 0;
    }

    bool
    has_line_gap() const
    {
        return (present & line_gap_bit) != 0;
    }

    bool
    has_minimum_line_height() const
    {
        return (present & minimum_line_height_bit) != 0;
    }

    bool
    has_provide_box() const
    {
        return (present & provide_box_bit) != 0;
    }
};

// empty container options
inline container_options const default_container{};

inline container_options
as_container_options(container_options const& options)
{
    return options;
}

inline container_options
as_container_options(container_layout_flag_set flags)
{
    return container_options{flags};
}

inline container_options
as_container_options(gap_spec const& g)
{
    container_options options;
    options.present = container_options::gap_bit;
    options.gap_value = g.value;
    return options;
}

inline container_options
as_container_options(line_gap_spec const& g)
{
    container_options options;
    options.present = container_options::line_gap_bit;
    options.line_gap_value = g.value;
    return options;
}

inline container_options
as_container_options(minimum_line_height_spec const& h)
{
    container_options options;
    options.present = container_options::minimum_line_height_bit;
    options.minimum_line_height_value = h.value;
    return options;
}

inline container_options
as_container_options(provide_box_spec const& p)
{
    container_options options;
    options.present = container_options::provide_box_bit;
    options.box = p.box;
    return options;
}

// concept for types that can be converted to `container_options`
template<class T>
concept container_like = requires(T const& t) {
    { as_container_options(t) } -> std::same_as<container_options>;
};

inline container_options&
operator|=(container_options& options, container_layout_flag_set flags)
{
    options.flags |= flags;
    return options;
}

inline container_options&
operator|=(container_options& options, gap_spec const& g)
{
    options.present |= container_options::gap_bit;
    options.gap_value = g.value;
    return options;
}

inline container_options&
operator|=(container_options& options, line_gap_spec const& g)
{
    options.present |= container_options::line_gap_bit;
    options.line_gap_value = g.value;
    return options;
}

inline container_options&
operator|=(container_options& options, minimum_line_height_spec const& h)
{
    options.present |= container_options::minimum_line_height_bit;
    options.minimum_line_height_value = h.value;
    return options;
}

inline container_options&
operator|=(container_options& options, provide_box_spec const& p)
{
    options.present |= container_options::provide_box_bit;
    options.box = p.box;
    return options;
}

inline container_options&
operator|=(container_options& options, container_options const& other)
{
    options.flags |= other.flags;
    if (other.has_gap())
    {
        options.present |= container_options::gap_bit;
        options.gap_value = other.gap_value;
    }
    if (other.has_line_gap())
    {
        options.present |= container_options::line_gap_bit;
        options.line_gap_value = other.line_gap_value;
    }
    if (other.has_minimum_line_height())
    {
        options.present |= container_options::minimum_line_height_bit;
        options.minimum_line_height_value = other.minimum_line_height_value;
    }
    if (other.has_provide_box())
    {
        options.present |= container_options::provide_box_bit;
        options.box = other.box;
    }
    return options;
}

inline container_options
operator|(container_options options, container_layout_flag_set flags)
{
    return options |= flags;
}

inline container_options
operator|(container_options options, gap_spec const& g)
{
    return options |= g;
}

inline container_options
operator|(container_options options, line_gap_spec const& g)
{
    return options |= g;
}

inline container_options
operator|(container_options options, minimum_line_height_spec const& h)
{
    return options |= h;
}

inline container_options
operator|(container_options options, provide_box_spec const& p)
{
    return options |= p;
}

inline container_options
operator|(container_options options, container_options const& other)
{
    return options |= other;
}

inline container_options
operator|(container_layout_flag_set flags, container_options const& options)
{
    return as_container_options(flags) | options;
}

inline container_options
operator|(gap_spec const& g, container_options const& options)
{
    return as_container_options(g) | options;
}

inline container_options
operator|(line_gap_spec const& g, container_options const& options)
{
    return as_container_options(g) | options;
}

inline container_options
operator|(minimum_line_height_spec const& h, container_options const& options)
{
    return as_container_options(h) | options;
}

inline container_options
operator|(provide_box_spec const& p, container_options const& options)
{
    return as_container_options(p) | options;
}

inline container_options
operator|(container_layout_flag_set flags, gap_spec const& g)
{
    return as_container_options(flags) | g;
}

inline container_options
operator|(gap_spec const& g, container_layout_flag_set flags)
{
    return as_container_options(g) | flags;
}

inline container_options
operator|(container_layout_flag_set flags, line_gap_spec const& g)
{
    return as_container_options(flags) | g;
}

inline container_options
operator|(line_gap_spec const& g, container_layout_flag_set flags)
{
    return as_container_options(g) | flags;
}

inline container_options
operator|(container_layout_flag_set flags, minimum_line_height_spec const& h)
{
    return as_container_options(flags) | h;
}

inline container_options
operator|(minimum_line_height_spec const& h, container_layout_flag_set flags)
{
    return as_container_options(h) | flags;
}

inline container_options
operator|(container_layout_flag_set flags, provide_box_spec const& p)
{
    return as_container_options(flags) | p;
}

inline container_options
operator|(provide_box_spec const& p, container_layout_flag_set flags)
{
    return as_container_options(p) | flags;
}

inline container_options
operator|(gap_spec const& g, line_gap_spec const& lg)
{
    return as_container_options(g) | lg;
}

inline container_options
operator|(line_gap_spec const& lg, gap_spec const& g)
{
    return as_container_options(lg) | g;
}

inline container_options
operator|(gap_spec const& g, minimum_line_height_spec const& h)
{
    return as_container_options(g) | h;
}

inline container_options
operator|(minimum_line_height_spec const& h, gap_spec const& g)
{
    return as_container_options(h) | g;
}

inline container_options
operator|(gap_spec const& g, provide_box_spec const& p)
{
    return as_container_options(g) | p;
}

inline container_options
operator|(provide_box_spec const& p, gap_spec const& g)
{
    return as_container_options(p) | g;
}

inline container_options
operator|(line_gap_spec const& lg, minimum_line_height_spec const& h)
{
    return as_container_options(lg) | h;
}

inline container_options
operator|(minimum_line_height_spec const& h, line_gap_spec const& lg)
{
    return as_container_options(h) | lg;
}

inline container_options
operator|(line_gap_spec const& lg, provide_box_spec const& p)
{
    return as_container_options(lg) | p;
}

inline container_options
operator|(provide_box_spec const& p, line_gap_spec const& lg)
{
    return as_container_options(p) | lg;
}

inline container_options
operator|(minimum_line_height_spec const& h, provide_box_spec const& p)
{
    return as_container_options(h) | p;
}

inline container_options
operator|(provide_box_spec const& p, minimum_line_height_spec const& h)
{
    return as_container_options(p) | h;
}

gap_spec
operator|(gap_spec, gap_spec) = delete;

line_gap_spec
operator|(line_gap_spec, line_gap_spec) = delete;

minimum_line_height_spec
operator|(minimum_line_height_spec, minimum_line_height_spec) = delete;

provide_box_spec
operator|(provide_box_spec, provide_box_spec) = delete;

// Build the ABI flag word for a container begin from placement flags and
// container-policy options.
inline alia_layout_flags_t
container_begin_flags(
    layout_flag_set placement, container_options const& options)
{
    alia_layout_flags_t code = raw_code(placement) | raw_code(options.flags);
    if (options.has_provide_box())
        code |= ALIA_PROVIDE_BOX;
    return code;
}

} // namespace alia
