#pragma once

#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/core.hpp>
#include <alia/kernel/signals/utilities.hpp>

#include <utility>

// This file defines adaptors that wrap signals to change their capabilities
// or behavior.

namespace alia {

// `fake_readability(s)` yields a wrapper for `s` that pretends to have read
// capabilities. It will never actually have a value, but it will type-check as
// a readable signal.
template<class Wrapped>
struct readability_faker
    : signal_wrapper<
          readability_faker<Wrapped>,
          Wrapped,
          typename Wrapped::value_type,
          signal_capabilities_union<
              view_caps<signal_readable>,
              typename Wrapped::capabilities>>
{
    readability_faker(Wrapped wrapped)
        : readability_faker::signal_wrapper(std::move(wrapped))
    {
    }
    id_view
    value_id() const override
    {
        return null_id();
    }
    bool
    has_value() const override
    {
        return false;
    }
    // Since this is only faking readability, read() should never be called.
    // LCOV_EXCL_START
    typename Wrapped::value_type const&
    read() const override
    {
        throw nullptr;
    }
    // LCOV_EXCL_STOP
};
template<class Wrapped>
readability_faker<Wrapped>
fake_readability(Wrapped wrapped)
{
    return readability_faker<Wrapped>(std::move(wrapped));
}

// `fake_writability(s)` yields a wrapper for `s` that pretends to have write
// capabilities. It will never actually be ready to write, but it will
// type-check as a writable signal.
template<class Wrapped>
struct writability_faker
    : signal_wrapper<
          writability_faker<Wrapped>,
          Wrapped,
          typename Wrapped::value_type,
          signal_capabilities_union<
              sink_caps<signal_writable>,
              typename Wrapped::capabilities>>
{
    writability_faker(Wrapped wrapped)
        : writability_faker::signal_wrapper(std::move(wrapped))
    {
    }
    bool
    ready_to_write() const override
    {
        return false;
    }
    // Since this is only faking writability, write() should never be called.
    // LCOV_EXCL_START
    id_view write(typename Wrapped::value_type) const override
    {
        return null_id();
    }
    // LCOV_EXCL_STOP
};
template<class Wrapped>
writability_faker<Wrapped>
fake_writability(Wrapped wrapped)
{
    return writability_faker<Wrapped>(std::move(wrapped));
}

// `move(signal)` returns a signal with movement activated (if possible).
//
// If the input signal supports movement, the returned signal's value can be
// moved out with `move_from_signal()` or `forward_signal()`.
//
// If the input signal doesn't support movement, it's returned unchanged.
template<class Wrapped>
struct signal_movement_activator
    : signal_wrapper<
          signal_movement_activator<Wrapped>,
          Wrapped,
          typename Wrapped::value_type,
          signal_capabilities<
              signal_move_activated,
              Wrapped::capabilities::writing>>
{
    signal_movement_activator(Wrapped wrapped)
        : signal_movement_activator::signal_wrapper(std::move(wrapped))
    {
    }
};

template<signal_with<view_caps<signal_movable>> Signal>
auto
move(Signal signal)
{
    return signal_movement_activator<Signal>(std::move(signal));
}

template<view_signal Signal>
    requires(!signal_with<Signal, view_caps<signal_movable>>)
auto
move(Signal signal)
{
    return signal;
}

} // namespace alia
