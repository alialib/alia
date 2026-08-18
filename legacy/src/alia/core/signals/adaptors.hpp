#ifndef ALIA_CORE_SIGNALS_ADAPTORS_HPP
#define ALIA_CORE_SIGNALS_ADAPTORS_HPP

#include <alia/core/context/interface.hpp>
#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/utilities.hpp>

namespace alia {

// signalize(x) turns x into a signal if it isn't already one.
// Or, in other words...
// signalize(s), where s is a signal, returns s.
// signalize(v), where v is a raw value, returns a value signal carrying v.
template<class Signal>
std::enable_if_t<is_readable_signal_type<Signal>::value, Signal>
signalize(Signal s)
{
    return std::move(s);
}
template<class Value, std::enable_if_t<!is_signal_type<Value>::value, int> = 0>
auto
signalize(Value v)
{
    return value(std::move(v));
}

// fake_readability(s), where :s is a signal, yields a wrapper for :s that
// pretends to have read capabilities. It will never actually have a value, but
// it will type-check as a readable signal.
template<class Wrapped>
struct readability_faker
    : signal_wrapper<
          readability_faker<Wrapped>,
          Wrapped,
          typename Wrapped::value_type,
          typename signal_capabilities_union<
              read_only_signal,
              typename Wrapped::capabilities>::type>
{
    readability_faker(Wrapped wrapped)
        : readability_faker::signal_wrapper(std::move(wrapped))
    {
    }
    id_interface const&
    value_id() const override
    {
        return null_id;
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
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnull-dereference"
#endif
        return *(typename Wrapped::value_type const*) nullptr;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
    }
    // LCOV_EXCL_STOP
};
template<class Wrapped>
readability_faker<Wrapped>
fake_readability(Wrapped wrapped)
{
    return readability_faker<Wrapped>(std::move(wrapped));
}

// fake_writability(s), where :s is a signal, yields a wrapper for :s that
// pretends to have write capabilities. It will never actually be ready to
// write, but it will type-check as a writable signal.
template<class Wrapped>
struct writability_faker
    : signal_wrapper<
          writability_faker<Wrapped>,
          Wrapped,
          typename Wrapped::value_type,
          typename signal_capabilities_union<
              write_only_signal,
              typename Wrapped::capabilities>::type>
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
    id_interface const& write(typename Wrapped::value_type) const override
    {
        return null_id;
    }
    // LCOV_EXCL_STOP
};
template<class Wrapped>
writability_faker<Wrapped>
fake_writability(Wrapped wrapped)
{
    return writability_faker<Wrapped>(std::move(wrapped));
}

// simplify_id(s), where :s is a signal, yields a wrapper for :s with the exact
// same read/write behavior but whose value ID is a simple_id (i.e., it is
// simply the value of the signal).
//
// The main utility of this is in cases where you have a signal carrying a
// small value with a complicated value ID (because it was picked from the
// signal of a larger data structure, for example). The more complicated ID
// might change superfluously.
//
template<class Wrapped>
struct simplified_id_wrapper
    : signal_wrapper<simplified_id_wrapper<Wrapped>, Wrapped>
{
    simplified_id_wrapper(Wrapped wrapped)
        : simplified_id_wrapper::signal_wrapper(std::move(wrapped))
    {
    }
    id_interface const&
    value_id() const override
    {
        if (this->has_value())
        {
            id_ = make_id_by_reference(this->read());
            return id_;
        }
        return null_id;
    }

 private:
    mutable simple_id_by_reference<typename Wrapped::value_type> id_;
};
template<class Wrapped>
simplified_id_wrapper<Wrapped>
simplify_id(Wrapped wrapped)
{
    return simplified_id_wrapper<Wrapped>(std::move(wrapped));
}

// minimize_id_changes(ctx, x), where :x is a signal, yields a new signal to
// x's value with a local ID that only changes when x's value actually changes.
template<class Value>
struct id_change_minimization_data
{
    captured_id input_id;
    counter_type version = 0;
    Value value;
    bool is_valid = false;
};
template<class Wrapped>
struct id_change_minimization_signal
    : signal_wrapper<id_change_minimization_signal<Wrapped>, Wrapped>
{
    id_change_minimization_signal(
        Wrapped wrapped,
        id_change_minimization_data<typename Wrapped::value_type>* data)
        : id_change_minimization_signal::signal_wrapper(std::move(wrapped)),
          data_(data)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = make_id(data_->version);
        return id_;
    }

 private:
    id_change_minimization_data<typename Wrapped::value_type>* data_;
    mutable simple_id<counter_type> id_;
};
template<class Signal>
void
update_id_change_minimization_data(
    id_change_minimization_data<typename Signal::value_type>* data,
    Signal const& x)
{
    if (!data->input_id.matches(x.value_id()))
    {
        // Only change the output ID if the value has actually changed.
        if (!(data->is_valid && signal_has_value(x)
              && data->value == read_signal(x)))
        {
            ++data->version;
            data->is_valid = false;
        }
        data->input_id.capture(x.value_id());
    }
    if (!data->is_valid && signal_has_value(x))
    {
        data->value = read_signal(x);
        data->is_valid = true;
    }
}
template<class Signal, class Value>
id_change_minimization_signal<Signal>
minimize_id_changes(
    dataless_core_context ctx,
    id_change_minimization_data<Value>* data,
    Signal x)
{
    if (is_refresh_event(ctx))
        update_id_change_minimization_data(data, x);
    return id_change_minimization_signal<Signal>(std::move(x), data);
}
template<class Signal>
id_change_minimization_signal<Signal>
minimize_id_changes(core_context ctx, Signal x)
{
    id_change_minimization_data<typename Signal::value_type>* data;
    get_cached_data(ctx, &data);
    return minimize_id_changes(ctx, data, std::move(x));
}

// move(signal) returns a signal with movement activated (if possible).
//
// If the input signal supports movement, the returned signal's value can be
// moved out with move_signal() or forward_signal().
//
// If the input signal doesn't support movement, it's returned unchanged.
//
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
    signal_movement_activator()
    {
    }
    signal_movement_activator(Wrapped wrapped)
        : signal_movement_activator::signal_wrapper(std::move(wrapped))
    {
    }
};
template<
    class Signal,
    std::enable_if_t<is_movable_signal_type<Signal>::value, int> = 0>
auto
move(Signal signal)
{
    return signal_movement_activator<Signal>(std::move(signal));
}
template<
    class Signal,
    std::enable_if_t<
        is_signal_type<Signal>::value && signal_is_readable<Signal>::value
            && !signal_is_movable<Signal>::value,
        int>
    = 0>
auto
move(Signal signal)
{
    return signal;
}

} // namespace alia

#endif
