#include <alia/timing/waves.hpp>

#include <alia/signals/adaptors.hpp>
#include <alia/signals/operators.hpp>
#include <alia/timing/timer.hpp>

namespace alia { namespace detail {

struct square_wave_data
{
    bool value = true;
    timer_data timer;
};

value_signal<bool>
square_wave(
    context ctx,
    readable<millisecond_count> true_duration,
    readable<millisecond_count> false_duration)
{
    auto& data = get_cached_data<square_wave_data>(ctx);

    timer timer(ctx, data.timer);

    if (timer.is_triggered())
        data.value = !data.value;

    if (!timer.is_active())
    {
        auto duration = conditional(
            data.value,
            true_duration,
            add_default(false_duration, true_duration));
        if (signal_has_value(duration))
        {
            timer.start(read_signal(duration));
        }
    }

    if (timer.is_triggered())
        abort_traversal(ctx);

    return value(data.value);
}

}} // namespace alia::detail
