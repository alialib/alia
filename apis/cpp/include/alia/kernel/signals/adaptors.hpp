#pragma once

#include <alia/kernel/actions/core.hpp>
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

// `signal_cast<Value>(x)` yields a proxy for `x` with the value type `Value`.
// The proxy applies `static_cast`s to convert its own values to and from
// `x`'s value type. If `Value` is already `x`'s value type, `x` is returned
// unchanged.
template<class Wrapped, class To>
struct casting_signal
    : casting_signal_wrapper<
          casting_signal<Wrapped, To>,
          Wrapped,
          To,
          signal_capabilities_intersection<
              typename Wrapped::capabilities,
              binding_caps<signal_move_activated, signal_clearable>>>
{
    casting_signal(Wrapped wrapped)
        : casting_signal::casting_signal_wrapper(std::move(wrapped))
    {
    }
    To const&
    read() const override
    {
        value_ = this->move_out();
        return value_;
    }
    To
    move_out() const override
    {
        return static_cast<To>(forward_signal(this->wrapped_));
    }
    To&
    destructive_ref() const override
    {
        value_ = this->move_out();
        return value_;
    }
    id_view
    write(To value) const override
    {
        return this->wrapped_.write(
            static_cast<typename Wrapped::value_type>(value));
    }

 private:
    mutable To value_;
};
template<class To, signal_type Wrapped>
auto
signal_cast(Wrapped wrapped)
{
    if constexpr (std::same_as<To, typename Wrapped::value_type>)
        return wrapped;
    else
        return casting_signal<Wrapped, To>(std::move(wrapped));
}

// `add_default(primary, default_)` yields a signal whose value is that of
// `primary` if it has one and that of `default_` otherwise.
// All writes go directly to `primary`. Either argument may be a signal or a
// raw value.
template<class Primary, class Default>
struct default_value_signal
    : signal_wrapper<
          default_value_signal<Primary, Default>,
          Primary,
          typename Primary::value_type,
          signal_capabilities<
              signal_capability_level_intersection<
                  Primary::capabilities::reading,
                  Default::capabilities::reading>,
              Primary::capabilities::writing>>
{
    default_value_signal(Primary primary, Default default_value)
        : default_value_signal::signal_wrapper(std::move(primary)),
          default_(std::move(default_value))
    {
    }
    bool
    has_value() const override
    {
        return this->wrapped_.has_value() || default_.has_value();
    }
    typename Primary::value_type const&
    read() const override
    {
        return this->wrapped_.has_value() ? this->wrapped_.read()
                                          : default_.read();
    }
    typename Primary::value_type
    move_out() const override
    {
        return this->wrapped_.has_value() ? this->wrapped_.move_out()
                                          : default_.move_out();
    }
    typename Primary::value_type&
    destructive_ref() const override
    {
        return this->wrapped_.has_value() ? this->wrapped_.destructive_ref()
                                          : default_.destructive_ref();
    }
    id_view
    value_id() const override
    {
        bool const using_primary = this->wrapped_.has_value();
        return make_id_pair(
            pair_,
            make_id(using_primary),
            using_primary ? this->wrapped_.value_id() : default_.value_id());
    }

 private:
    mutable alia_id_pair pair_{};
    Default default_;
};
template<class Primary, class Default>
auto
add_default(Primary primary, Default default_)
{
    auto primary_signal = signalize(std::move(primary));
    auto default_signal = signalize(std::move(default_));
    return default_value_signal<
        decltype(primary_signal),
        decltype(default_signal)>(
        std::move(primary_signal), std::move(default_signal));
}

// `has_value_view(s)` yields a view to a boolean that is true iff `s`
// currently has a value. The returned signal always has a value.
template<class Wrapped>
struct has_value_view_signal
    : regular_signal<
          has_value_view_signal<Wrapped>,
          bool,
          view_caps<signal_readable>>
{
    has_value_view_signal(Wrapped wrapped) : wrapped_(std::move(wrapped))
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    bool const&
    read() const override
    {
        value_ = wrapped_.has_value();
        return value_;
    }

 private:
    Wrapped wrapped_;
    mutable bool value_;
};
template<view_signal Wrapped>
auto
has_value_view(Wrapped wrapped)
{
    return has_value_view_signal<Wrapped>(std::move(wrapped));
}

// `ready_to_write_view(s)` yields a view to a boolean that is true iff `s` is
// currently ready to write. The returned signal always has a value.
template<class Wrapped>
struct ready_to_write_view_signal
    : regular_signal<
          ready_to_write_view_signal<Wrapped>,
          bool,
          view_caps<signal_readable>>
{
    ready_to_write_view_signal(Wrapped wrapped) : wrapped_(std::move(wrapped))
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    bool const&
    read() const override
    {
        value_ = wrapped_.ready_to_write();
        return value_;
    }

 private:
    Wrapped wrapped_;
    mutable bool value_;
};
template<signal_type Wrapped>
auto
ready_to_write_view(Wrapped wrapped)
{
    return ready_to_write_view_signal<Wrapped>(std::move(wrapped));
}

// `mask(signal, flag)` is empty unless `flag` is true, in which case it is
// equivalent to `signal`. `flag` may be a signal or a raw value.
template<class Primary, class Mask>
struct masking_signal : signal_wrapper<masking_signal<Primary, Mask>, Primary>
{
    masking_signal(Primary primary, Mask mask)
        : masking_signal::signal_wrapper(std::move(primary)),
          mask_(std::move(mask))
    {
    }
    bool
    has_value() const override
    {
        return mask_.has_value() && mask_.read() && this->wrapped_.has_value();
    }
    id_view
    value_id() const override
    {
        if (mask_.has_value() && mask_.read())
            return this->wrapped_.value_id();
        return null_id();
    }
    bool
    ready_to_write() const override
    {
        return mask_.has_value() && mask_.read()
            && this->wrapped_.ready_to_write();
    }

 private:
    Mask mask_;
};
template<class Signal, class AvailabilityFlag>
    requires(!action_type<Signal>)
auto
mask(Signal signal, AvailabilityFlag availability_flag)
{
    auto signalized = signalize(std::move(signal));
    auto flag = signalize(std::move(availability_flag));
    return masking_signal<decltype(signalized), decltype(flag)>(
        std::move(signalized), std::move(flag));
}

// `mask_writes(signal, flag)` keeps `signal`'s read behavior but only allows
// writes when `flag` is true. Capabilities are unchanged.
template<class Primary, class Mask>
struct write_masking_signal
    : signal_wrapper<write_masking_signal<Primary, Mask>, Primary>
{
    write_masking_signal(Primary primary, Mask mask)
        : write_masking_signal::signal_wrapper(std::move(primary)),
          mask_(std::move(mask))
    {
    }
    bool
    ready_to_write() const override
    {
        return mask_.has_value() && mask_.read()
            && this->wrapped_.ready_to_write();
    }

 private:
    Mask mask_;
};
template<signal_type Signal, class WritabilityFlag>
auto
mask_writes(Signal signal, WritabilityFlag writability_flag)
{
    auto flag = signalize(std::move(writability_flag));
    return write_masking_signal<Signal, decltype(flag)>(
        std::move(signal), std::move(flag));
}

// `disable_writes(s)` yields a wrapper for `s` with writes disabled.
template<signal_type Signal>
auto
disable_writes(Signal s)
{
    return mask_writes(std::move(s), false);
}

// `mask_reads(signal, flag)` keeps `signal`'s write behavior but only has a
// value when `flag` is true. Capabilities are unchanged.
template<class Primary, class Mask>
struct read_masking_signal
    : signal_wrapper<read_masking_signal<Primary, Mask>, Primary>
{
    read_masking_signal(Primary primary, Mask mask)
        : read_masking_signal::signal_wrapper(std::move(primary)),
          mask_(std::move(mask))
    {
    }
    bool
    has_value() const override
    {
        return mask_.has_value() && mask_.read() && this->wrapped_.has_value();
    }

 private:
    Mask mask_;
};
template<signal_type Signal, class ReadabilityFlag>
auto
mask_reads(Signal signal, ReadabilityFlag readability_flag)
{
    auto flag = signalize(std::move(readability_flag));
    return read_masking_signal<Signal, decltype(flag)>(
        std::move(signal), std::move(flag));
}

// `disable_reads(s)` yields a wrapper for `s` with reads disabled.
template<signal_type Signal>
auto
disable_reads(Signal s)
{
    return mask_reads(std::move(s), false);
}

// `unwrap(signal)`, where `signal` carries a `std::optional` value, yields a
// signal that directly carries the value wrapped inside the optional.
// If the optional signal is writable, the unwrapped signal is clearable.
// (Clearing the signal sets the optional to `std::nullopt`.)
template<class Wrapped>
using unwrapper_signal_capabilities = signal_capabilities<
    Wrapped::capabilities::reading,
    Wrapped::capabilities::writing == signal_unwritable ? signal_unwritable
                                                        : signal_clearable>;

template<class Wrapped>
struct unwrapper_signal
    : casting_signal_wrapper<
          unwrapper_signal<Wrapped>,
          Wrapped,
          typename Wrapped::value_type::value_type,
          unwrapper_signal_capabilities<Wrapped>>
{
    unwrapper_signal(Wrapped wrapped)
        : unwrapper_signal::casting_signal_wrapper(std::move(wrapped))
    {
    }
    bool
    has_value() const override
    {
        return this->wrapped_.has_value() && this->wrapped_.read().has_value();
    }
    typename Wrapped::value_type::value_type const&
    read() const override
    {
        return this->wrapped_.read().value();
    }
    typename Wrapped::value_type::value_type
    move_out() const override
    {
        return *this->wrapped_.move_out();
    }
    typename Wrapped::value_type::value_type&
    destructive_ref() const override
    {
        return *this->wrapped_.destructive_ref();
    }
    id_view
    value_id() const override
    {
        if (this->has_value())
            return this->wrapped_.value_id();
        return null_id();
    }
    id_view
    write(typename Wrapped::value_type::value_type value) const override
    {
        return this->wrapped_.write(std::move(value));
    }
    void
    clear() const override
    {
        this->wrapped_.write(typename Wrapped::value_type());
    }
};
template<view_signal Signal>
auto
unwrap(Signal signal)
{
    return unwrapper_signal<Signal>(std::move(signal));
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

// A radio signal models the semantics of a radio button.
//
// `make_radio_signal(selected, index)` yields a boolean binding that is true
// iff the value of `selected` matches the value of `index`. Writing `true` to
// the signal sets `selected` to `index`. (Writing `false` is considered
// a meaningless operation.)
//
template<class Selected, class Index>
struct radio_signal
    : lazy_signal<
          radio_signal<Selected, Index>,
          bool,
          binding_caps<signal_readable>>
{
    radio_signal(Selected selected, Index index)
        : selected_(std::move(selected)), index_(std::move(index))
    {
    }
    bool
    has_value() const override
    {
        return signal_has_value(selected_) && signal_has_value(index_);
    }
    bool
    move_out() const override
    {
        return read_signal(selected_) == read_signal(index_);
    }
    id_view
    value_id() const override
    {
        return selected_.value_id();
    }
    bool
    ready_to_write() const override
    {
        return signal_ready_to_write(selected_) && signal_has_value(index_);
    }
    id_view
    write(bool) const override
    {
        write_signal(selected_, read_signal(index_));
        return null_id();
    }

 private:
    Selected selected_;
    Index index_;
};
template<class Selected, class Index>
    requires view_signal<Selected> && sink_signal<Selected>
          && view_signal<Index>
radio_signal<Selected, Index>
make_radio_signal(Selected selected, Index index)
{
    return radio_signal<Selected, Index>(
        std::move(selected), std::move(index));
}

} // namespace alia
