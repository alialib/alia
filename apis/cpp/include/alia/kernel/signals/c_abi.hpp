#pragma once

#include <alia/abi/kernel/signal.h>
#include <alia/kernel/signals/core.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

// This file defines utilities for connecting the C++ signals system to the C
// ABI's signal structs.

namespace alia {

// This concept tests if a type matches the Alia C ABI pattern for a
// bidirectional signal that stores its value inline.
template<class CSignal>
concept inline_c_signal = requires(CSignal s, CSignal const& cs) {
    s.flags = alia_signal_flags{};
    s.value;
    cs.flags;
    cs.value;
};

template<inline_c_signal CSignal>
using inline_c_signal_value_t
    = std::remove_cvref_t<decltype(std::declval<CSignal>().value)>;

// Convert a C++ signal into an inline C signal.
template<inline_c_signal CSignal, view_signal Signal>
    requires std::convertible_to<
        typename Signal::value_type,
        inline_c_signal_value_t<CSignal>>
CSignal
to_inline_c_signal(Signal const& signal)
{
    CSignal c{};
    c.flags = 0;
    if (signal_has_value(signal))
    {
        c.flags |= ALIA_SIGNAL_READABLE;
        c.value = static_cast<inline_c_signal_value_t<CSignal>>(
            read_signal(signal));
    }
    if constexpr (sink_signal<Signal>)
    {
        if (signal_ready_to_write(signal))
            c.flags |= ALIA_SIGNAL_WRITABLE;
    }
    return c;
}

// If the given inline C signal was written to, write the value back to the
// corresponding C++ signal.
template<binding_signal Signal, inline_c_signal CSignal>
    requires std::convertible_to<
        inline_c_signal_value_t<CSignal>,
        typename Signal::value_type>
void
write_back_c_signal(alia_context* ctx, Signal const& signal, CSignal const& c)
{
    if (c.flags & ALIA_SIGNAL_WRITTEN)
    {
        write_signal(
            ctx, signal, static_cast<typename Signal::value_type>(c.value));
    }
}

} // namespace alia
