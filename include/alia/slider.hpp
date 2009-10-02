#ifndef ALIA_SLIDER_HPP
#define ALIA_SLIDER_HPP

#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <alia/flags.hpp>
#include <limits>

namespace alia {

template<class T>
struct slider_result
{
    bool changed;
    T new_value;

    // allows use within if statements without other unintended conversions
    typedef bool slider_result::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return changed ? &slider_result::changed : 0; }
};

namespace impl {
    bool do_slider(context& ctx, double* value, double minimum, double maximum,
        double step, bool integer, unsigned flags, layout const& layout_spec);
}

template<class T>
slider_result<T> do_slider(context& ctx, accessor<T> const& value,
    T minimum, T maximum, T step = 0, unsigned flags = 0,
    layout const& layout_spec = default_layout)
{
    // TODO: What should this do when the value is invalid?
    T current_value = value.is_valid() ? value.get() : 0;
    double value_as_double = double(current_value);
    bool is_integer = std::numeric_limits<T>::is_integer;
    if (impl::do_slider(ctx, &value_as_double, double(minimum),
        double(maximum), double(step), is_integer, flags, layout_spec))
    {
        T new_value = T(is_integer ? value_as_double + 0.5 : value_as_double);
        if (new_value != current_value)
        {
            value.set(new_value);
            slider_result<T> r;
            r.changed = true;
            r.new_value = new_value;
            return r;
        }
    }
    slider_result<T> r;
    r.changed = false;
    return r;
}

}

#endif
