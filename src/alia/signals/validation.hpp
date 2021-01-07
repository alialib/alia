#ifndef ALIA_SIGNALS_VALIDATION_HPP
#define ALIA_SIGNALS_VALIDATION_HPP

#include <alia/signals/core.hpp>

namespace alia {

struct signal_validation_data
{
    captured_id value_id;
    std::exception_ptr error;
};

template<class Wrapped>
struct validated_signal : signal_wrapper<validated_signal<Wrapped>, Wrapped>
{
    validated_signal()
    {
    }
    validated_signal(signal_validation_data* data, Wrapped wrapped)
        : validated_signal::signal_wrapper(wrapped), data_(data)
    {
    }
    bool
    has_value() const override final
    {
        return !data_->error && this->wrapped_.has_value();
    }
    bool
    invalidate(std::exception_ptr error) const override final
    {
        data_->error = error;
        this->wrapped_.invalidate(error);
        return true;
    }
    bool
    is_invalidated() const override final
    {
        return data_->error || this->wrapped_.is_invalidated();
    }

 private:
    signal_validation_data* data_;
};

template<class Signal>
auto
enforce_validity(
    dataless_context ctx, Signal signal, signal_validation_data& data)
{
    on_refresh(ctx, [&](auto) {
        if (!signal.is_invalidated()
            && !data.value_id.matches(signal.value_id()))
        {
            data.error = nullptr;
            data.value_id.capture(signal.value_id());
        }
    });
    return validated_signal<Signal>(&data, signal);
}

template<class Signal>
auto
enforce_validity(context ctx, Signal signal)
{
    auto& data = get_cached_data<signal_validation_data>(ctx);
    return enforce_validity(ctx, signal, data);
}

} // namespace alia

#endif
