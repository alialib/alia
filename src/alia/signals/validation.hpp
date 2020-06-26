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
struct validated_signal : signal<
                              validated_signal<Wrapped>,
                              typename Wrapped::value_type,
                              typename Wrapped::direction_tag>
{
    validated_signal()
    {
    }
    validated_signal(signal_validation_data* data, Wrapped wrapped)
        : data_(data), wrapped_(wrapped)
    {
    }
    bool
    has_value() const
    {
        return !data_->error && wrapped_.has_value();
    }
    typename Wrapped::value_type const&
    read() const
    {
        return wrapped_.read();
    }
    id_interface const&
    value_id() const
    {
        return wrapped_.value_id();
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
    bool
    invalidate(std::exception_ptr error) const
    {
        data_->error = error;
        wrapped_.invalidate(error);
        return true;
    }
    bool
    is_invalidated() const
    {
        return data_->error || wrapped_.is_invalidated();
    }

 private:
    signal_validation_data* data_;
    Wrapped wrapped_;
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
