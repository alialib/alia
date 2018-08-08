#ifndef ALIA_SIGNALS_NUMERIC_HPP
#define ALIA_SIGNALS_NUMERIC_HPP

#include <alia/signals/application.hpp>
#include <alia/signals/utilities.hpp>

#include <cmath>

// This file defines some numerical adaptors for signals.

namespace alia {

// scale(a, factor) creates a new signal that presents a scaled view of a,
// where a is a signal to a numeric value.
template<class Wrapped>
struct scaling_signal_wrapper : regular_signal<
                                    scaling_signal_wrapper<Wrapped>,
                                    typename Wrapped::value_type,
                                    typename Wrapped::direction_tag>
{
    typedef typename Wrapped::value_type wrapped_value_type;
    scaling_signal_wrapper(Wrapped wrapped, wrapped_value_type scale_factor)
        : wrapped_(wrapped), scale_factor_(scale_factor)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    wrapped_value_type const&
    read() const
    {
        return lazy_reader_.read(
            [&] { return wrapped_.read() * scale_factor_; });
    }
    bool
    is_writable() const
    {
        return wrapped_.is_writable();
    }
    void
    write(wrapped_value_type const& value) const
    {
        wrapped_.write(value / scale_factor_);
    }

 private:
    Wrapped wrapped_;
    wrapped_value_type scale_factor_;
    lazy_reader<wrapped_value_type> lazy_reader_;
};
template<class Wrapped>
scaling_signal_wrapper<Wrapped>
scale(Wrapped const& wrapped, typename Wrapped::value_type scale_factor)
{
    return scaling_signal_wrapper<Wrapped>(wrapped, scale_factor);
}

// offset(a, offset) presents an offset view of a, where a is a signal
// to a numeric value.
template<class Wrapped>
struct offset_signal_wrapper : regular_signal<
                                   offset_signal_wrapper<Wrapped>,
                                   typename Wrapped::value_type,
                                   typename Wrapped::direction_tag>
{
    typedef typename Wrapped::value_type wrapped_value_type;
    offset_signal_wrapper(Wrapped wrapped, typename Wrapped::value_type offset)
        : wrapped_(wrapped), offset_(offset)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    wrapped_value_type const&
    read() const
    {
        return lazy_reader_.read([&] { return wrapped_.read() + offset_; });
    }
    bool
    is_writable() const
    {
        return wrapped_.is_writable();
    }
    void
    write(typename Wrapped::value_type const& value) const
    {
        wrapped_.write(value - offset_);
    }

 private:
    Wrapped wrapped_;
    wrapped_value_type offset_;
    lazy_reader<wrapped_value_type> lazy_reader_;
};
template<class Wrapped>
offset_signal_wrapper<Wrapped>
offset(Wrapped const& wrapped, typename Wrapped::value_type offset)
{
    return offset_signal_wrapper<Wrapped>(wrapped, offset);
}

// round_signal_writes(signal, step) yields a wrapper which rounds any writes to
// :signal so that values are always a multiple of :step.
template<class Wrapped>
struct rounding_signal_wrapper : regular_signal<
                                     rounding_signal_wrapper<Wrapped>,
                                     typename Wrapped::value_type,
                                     typename Wrapped::direction_tag>
{
    rounding_signal_wrapper(Wrapped wrapped, typename Wrapped::value_type step)
        : wrapped_(wrapped), step_(step)
    {
    }
    bool
    is_readable() const
    {
        return wrapped_.is_readable();
    }
    typename Wrapped::value_type const&
    read() const
    {
        return wrapped_.read();
    }
    bool
    is_writable() const
    {
        return wrapped_.is_writable();
    }
    void
    write(typename Wrapped::value_type const& value) const
    {
        wrapped_.write(
            std::floor(value / step_ + typename Wrapped::value_type(0.5))
            * step_);
    }

 private:
    Wrapped wrapped_;
    typename Wrapped::value_type step_;
};
template<class Wrapped>
rounding_signal_wrapper<Wrapped>
round_signal_writes(Wrapped const& wrapped, typename Wrapped::value_type step)
{
    return rounding_signal_wrapper<Wrapped>(wrapped, step);
}

} // namespace alia

#endif
