#ifndef ALIA_SIGNALS_NUMERIC_HPP
#define ALIA_SIGNALS_NUMERIC_HPP

#include <alia/signals/adaptors.hpp>
#include <alia/signals/application.hpp>
#include <alia/signals/utilities.hpp>

#include <cmath>

// This file defines some numerical adaptors for signals.

namespace alia {

// scale(n, factor) creates a new signal that presents a scaled view of :n,
// where :n and :factor are both numeric signals.
template<class N, class Factor>
struct scaled_signal : regular_signal<
                           scaled_signal<N, Factor>,
                           typename N::value_type,
                           typename N::direction_tag>
{
    scaled_signal(N n, Factor scale_factor) : n_(n), scale_factor_(scale_factor)
    {
    }
    bool
    has_value() const
    {
        return n_.has_value() && scale_factor_.has_value();
    }
    typename N::value_type const&
    read() const
    {
        return lazy_reader_.read(
            [&] { return n_.read() * scale_factor_.read(); });
    }
    bool
    ready_to_write() const
    {
        return n_.ready_to_write() && scale_factor_.has_value();
    }
    void
    write(typename N::value_type value) const
    {
        n_.write(value / scale_factor_.read());
    }
    bool
    invalidate(std::exception_ptr error) const
    {
        return n_.invalidate(error);
    }
    bool
    is_invalidated() const
    {
        return n_.is_invalidated();
    }

 private:
    N n_;
    Factor scale_factor_;
    lazy_reader<typename N::value_type> lazy_reader_;
};
template<class N, class Factor>
scaled_signal<N, Factor>
make_scaled_signal(N n, Factor scale_factor)
{
    return scaled_signal<N, Factor>(n, scale_factor);
}
template<class N, class Factor>
auto
scale(N n, Factor scale_factor)
{
    return make_scaled_signal(n, signalize(scale_factor));
}

// offset(n, offset) presents an offset view of :n.
template<class N, class Offset>
struct offset_signal : regular_signal<
                           offset_signal<N, Offset>,
                           typename N::value_type,
                           typename N::direction_tag>
{
    offset_signal(N n, Offset offset) : n_(n), offset_(offset)
    {
    }
    bool
    has_value() const
    {
        return n_.has_value() && offset_.has_value();
    }
    typename N::value_type const&
    read() const
    {
        return lazy_reader_.read([&] { return n_.read() + offset_.read(); });
    }
    bool
    ready_to_write() const
    {
        return n_.ready_to_write() && offset_.has_value();
    }
    void
    write(typename N::value_type value) const
    {
        n_.write(value - offset_.read());
    }
    bool
    invalidate(std::exception_ptr error) const
    {
        return n_.invalidate(error);
    }
    bool
    is_invalidated() const
    {
        return n_.is_invalidated();
    }

 private:
    N n_;
    Offset offset_;
    lazy_reader<typename N::value_type> lazy_reader_;
};
template<class N, class Offset>
offset_signal<N, Offset>
make_offset_signal(N n, Offset offset)
{
    return offset_signal<N, Offset>(n, offset);
}
template<class N, class Offset>
auto
offset(N n, Offset offset)
{
    return make_offset_signal(n, signalize(offset));
}

// round_signal_writes(n, step) yields a wrapper which rounds any writes to
// :n so that values are always a multiple of :step.
template<class N, class Step>
struct rounding_signal_wrapper : regular_signal<
                                     rounding_signal_wrapper<N, Step>,
                                     typename N::value_type,
                                     typename N::direction_tag>
{
    rounding_signal_wrapper(N n, Step step) : n_(n), step_(step)
    {
    }
    bool
    has_value() const
    {
        return n_.has_value();
    }
    typename N::value_type const&
    read() const
    {
        return n_.read();
    }
    bool
    ready_to_write() const
    {
        return n_.ready_to_write() && step_.has_value();
    }
    void
    write(typename N::value_type value) const
    {
        typename N::value_type step = step_.read();
        n_.write(std::floor(value / step + typename N::value_type(0.5)) * step);
    }
    bool
    invalidate(std::exception_ptr error) const
    {
        return n_.invalidate(error);
    }
    bool
    is_invalidated() const
    {
        return n_.is_invalidated();
    }

 private:
    N n_;
    Step step_;
};
template<class N, class Step>
rounding_signal_wrapper<N, Step>
make_rounding_signal_wrapper(N n, Step step)
{
    return rounding_signal_wrapper<N, Step>(n, step);
}
template<class N, class Step>
auto
round_signal_writes(N n, Step step)
{
    return make_rounding_signal_wrapper(n, signalize(step));
}

} // namespace alia

#endif
