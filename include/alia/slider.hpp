#ifndef ALIA_SLIDER_HPP
#define ALIA_SLIDER_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <alia/flags.hpp>
#include <limits>

namespace alia {

template<class T>
struct slider_result : control_result<T>
{};

namespace impl {
    bool do_slider(context& ctx, bool* valid, double* value, double minimum,
        double maximum, double step, bool integer, layout const& layout_spec,
        flag_set flags);
}

// accepted flags:
// HORIZONTAL, VERTICAL (mutually exclusive, default is HORIZONTAL)
template<class T>
slider_result<T> do_slider(context& ctx, accessor<T> const& value,
    T minimum, T maximum, T step = 0,
    layout const& layout_spec = default_layout, flag_set flags = NO_FLAGS)
{
    bool const initially_valid = value.is_valid();
    T const initial_value = value.get();
    bool valid = initially_valid;
    double value_as_double;
    if (valid)
    {
        if (initial_value < minimum || initial_value > maximum)
            valid = false;
        else
            value_as_double = double(initial_value);
    }
    bool is_integer = std::numeric_limits<T>::is_integer;
    if (impl::do_slider(ctx, &valid, &value_as_double, double(minimum),
        double(maximum), double(step), is_integer, layout_spec, flags))
    {
        if (valid)
        {
            T new_value =
                T(is_integer ? value_as_double + 0.5 : value_as_double);
            if (!initially_valid || new_value != initial_value)
            {
                value.set(new_value);
                slider_result<T> r;
                r.changed = true;
                r.new_value = new_value;
                return r;
            }
        }
    }
    slider_result<T> r;
    r.changed = false;
    return r;
}

}

#endif
