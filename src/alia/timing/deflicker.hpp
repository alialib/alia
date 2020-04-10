#ifndef ALIA_TIMING_DEFLICKER_HPP
#define ALIA_TIMING_DEFLICKER_HPP

#include <alia/signals/adaptors.hpp>
#include <alia/timing/timer.hpp>

namespace alia {

// The following is a small utility used for deflickering...

template<class Value>
struct captured_value
{
    bool valid = false;
    Value value;
    captured_id id;
};

template<class Value>
void
clear(captured_value<Value>& captured)
{
    captured.valid = false;
    captured.id.clear();
}

template<class Value, class Signal>
void
capture(captured_value<Value>& captured, Signal const& signal)
{
    captured.value = signal.read();
    captured.id.capture(signal.value_id());
    captured.valid = true;
}

// deflicker(ctx, x, delay) returns a deflickered version of the signal :x.
//
// Whenever :x has a value, the deflickered signal carries the same value.
// Whenever :x loses its value, the deflickered signal retains the old value for
// a period of time, given by :delay. (If a new value arrives on :x during this
// period, it will pick that up instead.)
//
// :delay is specified in milliseconds and can be either a raw value or a
// signal. It defaults to 250 ms.
//

unsigned const default_deflicker_delay = 250;

template<class Value>
struct deflickering_data
{
    timer_data timer;
    captured_value<Value> captured;
};

template<class Wrapped>
struct deflickering_signal : signal<
                                 deflickering_signal<Wrapped>,
                                 typename Wrapped::value_type,
                                 typename Wrapped::direction_tag>
{
    deflickering_signal()
    {
    }
    deflickering_signal(
        Wrapped wrapped, captured_value<typename Wrapped::value_type>& captured)
        : wrapped_(wrapped), captured_(captured)
    {
    }
    bool
    has_value() const
    {
        return captured_.valid;
    }
    typename Wrapped::value_type const&
    read() const
    {
        return captured_.value;
    }
    id_interface const&
    value_id() const
    {
        return captured_.id.is_initialized() ? captured_.id.get() : null_id;
    }
    bool
    ready_to_write() const
    {
        return wrapped_.ready_to_write();
    }
    void
    write(typename Wrapped::value_type const& value) const
    {
        wrapped_.write(value);
    }

 private:
    Wrapped wrapped_;
    captured_value<typename Wrapped::value_type>& captured_;
};

template<class Signal, class Value, class Delay = millisecond_count>
deflickering_signal<Signal>
deflicker(
    dataless_context ctx,
    deflickering_data<Value>& data,
    Signal x,
    Delay delay = default_deflicker_delay)
{
    auto delay_signal = signalize(delay);

    raw_timer timer(ctx, data.timer);
    if (timer.is_triggered())
    {
        // If the timer is triggered, it means we were holding a stale value and
        // it's time to clear it out.
        clear(data.captured);
        abort_traversal(ctx);
    }

    on_refresh(ctx, [&](auto) {
        if (x.has_value())
        {
            if (x.value_id() != data.captured.id)
            {
                // :x is carrying a different value than the one we have
                // captured, so capture the new one.
                capture(data.captured, x);
                // If we had an active timer, stop it.
                timer.stop();
            }
        }
        else
        {
            if (data.captured.id.is_initialized() && !timer.is_active())
            {
                // :x has no value, and this is apparently the first time we've
                // noticed that, so start the timer.
                if (signal_has_value(delay_signal))
                {
                    timer.start(read_signal(delay_signal));
                }
                else
                {
                    // If the delay isn't readable, we can't start the timer, so
                    // just drop the value immediately.
                    clear(data.captured);
                }
            }
        }
    });

    return deflickering_signal<Signal>(x, data.captured);
}

template<class Signal, class Delay = millisecond_count>
deflickering_signal<Signal>
deflicker(context ctx, Signal x, Delay delay = default_deflicker_delay)
{
    typedef typename Signal::value_type value_type;
    auto& data = get_cached_data<deflickering_data<value_type>>(ctx);
    return deflicker(ctx, data, x, delay);
}

} // namespace alia

#endif
