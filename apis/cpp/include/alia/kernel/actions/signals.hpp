#pragma once

#include <alia/kernel/actions/core.hpp>
#include <alia/kernel/signals/core.hpp>
#include <alia/kernel/signals/utilities.hpp>

#include <utility>

// This file provides action-centric adaptors that operate on signals.

namespace alia {

// add_write_action(signal, on_write) wraps signal in a similar signal that
// will invoke on_write whenever the signal is written to. (on_write will be
// passed the value that was written to the signal.)
template<class Wrapped, class OnWrite>
struct write_action_signal
    : signal_wrapper<
          write_action_signal<Wrapped, OnWrite>,
          Wrapped,
          typename Wrapped::value_type,
          signal_capabilities_union<
              sink_caps<signal_writable>,
              typename Wrapped::capabilities>>
{
    write_action_signal(Wrapped wrapped, OnWrite on_write)
        : write_action_signal::signal_wrapper(std::move(wrapped)),
          on_write_(std::move(on_write))
    {
    }
    bool
    ready_to_write() const override
    {
        return this->wrapped_.ready_to_write() && on_write_.is_ready();
    }
    id_view
    write(typename Wrapped::value_type value) const override
    {
        perform_action(on_write_, value);
        return this->wrapped_.write(std::move(value));
    }

 private:
    OnWrite on_write_;
};
template<class Wrapped, class OnWrite>
auto
add_write_action(Wrapped wrapped, OnWrite on_write)
{
    return write_action_signal<Wrapped, OnWrite>(
        std::move(wrapped), std::move(on_write));
}

} // namespace alia
