#pragma once

#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/utilities.hpp>

#include <cmath>
#include <utility>

// This file defines numerical adaptors for signals.

namespace alia {

// `scale(n, factor)` yields a scaled view of `n`. `factor` can either be a
// signal or a raw value. Writes divide by `factor` and go to `n`.
template<class N, class Factor>
struct scaled_signal
    : lazy_signal_wrapper<
          scaled_signal<N, Factor>,
          N,
          typename N::value_type,
          signal_capabilities<signal_move_activated, N::capabilities::writing>,
          typename N::value_type>
{
    scaled_signal(N n, Factor scale_factor)
        : scaled_signal::lazy_signal_wrapper(std::move(n)),
          scale_factor_(std::move(scale_factor))
    {
    }
    bool
    has_value() const override
    {
        return this->wrapped_.has_value() && scale_factor_.has_value();
    }
    typename N::value_type
    move_out() const override
    {
        return this->wrapped_.read() * scale_factor_.read();
    }
    typename N::value_type const&
    value_id() const
    {
        return this->read();
    }
    bool
    ready_to_write() const override
    {
        return this->wrapped_.ready_to_write() && scale_factor_.has_value();
    }
    void
    write(typename N::value_type value) const override
    {
        this->wrapped_.write(value / forward_signal(scale_factor_));
    }

 private:
    Factor scale_factor_;
};
template<view_signal N, class Factor>
auto
scale(N n, Factor scale_factor)
{
    auto factor = signalize(std::move(scale_factor));
    return scaled_signal<N, decltype(factor)>(std::move(n), std::move(factor));
}

// `offset(n, delta)` yields a view of `n` shifted by `delta`. `delta` can
// either be a signal or a raw value. Writes subtract `delta` and go to `n`.
template<class N, class Delta>
struct offset_signal
    : lazy_signal_wrapper<
          offset_signal<N, Delta>,
          N,
          typename N::value_type,
          signal_capabilities<signal_move_activated, N::capabilities::writing>,
          typename N::value_type>
{
    offset_signal(N n, Delta delta)
        : offset_signal::lazy_signal_wrapper(std::move(n)),
          delta_(std::move(delta))
    {
    }
    bool
    has_value() const override
    {
        return this->wrapped_.has_value() && delta_.has_value();
    }
    typename N::value_type
    move_out() const override
    {
        return this->wrapped_.read() + delta_.read();
    }
    typename N::value_type const&
    value_id() const
    {
        return this->read();
    }
    bool
    ready_to_write() const override
    {
        return this->wrapped_.ready_to_write() && delta_.has_value();
    }
    void
    write(typename N::value_type value) const override
    {
        this->wrapped_.write(value - forward_signal(delta_));
    }

 private:
    Delta delta_;
};
template<view_signal N, class Delta>
auto
offset(N n, Delta delta)
{
    auto delta_signal = signalize(std::move(delta));
    return offset_signal<N, decltype(delta_signal)>(
        std::move(n), std::move(delta_signal));
}

// `round_signal_writes(n, step)` yields a wrapper for `n` that rounds writes
// to a multiple of `step`. `step` can either be a signal or a raw value. Reads
// are unaffected.
template<class N, class Step>
struct rounding_signal_wrapper
    : signal_wrapper<rounding_signal_wrapper<N, Step>, N>
{
    rounding_signal_wrapper(N n, Step step)
        : rounding_signal_wrapper::signal_wrapper(std::move(n)),
          step_(std::move(step))
    {
    }
    bool
    ready_to_write() const override
    {
        return this->wrapped_.ready_to_write() && step_.has_value();
    }
    void
    write(typename N::value_type value) const override
    {
        auto step = step_.read();
        this->wrapped_.write(
            std::floor(value / step + typename N::value_type(0.5)) * step);
    }

 private:
    Step step_;
};
template<signal_type N, class Step>
auto
round_signal_writes(N n, Step step)
{
    auto step_signal = signalize(std::move(step));
    return rounding_signal_wrapper<N, decltype(step_signal)>(
        std::move(n), std::move(step_signal));
}

} // namespace alia
