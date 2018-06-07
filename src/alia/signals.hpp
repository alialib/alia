#ifndef ALIA_SIGNALS_HPP
#define ALIA_SIGNALS_HPP

#include <alia/common.hpp>
#include <alia/id.hpp>

namespace alia {

// Signals are passed by const reference into UI functions.
// They're typically created directly at the call site as function arguments
// and are only valid for the life of the function call.
// Signals wrappers are templated and store copies of the actual wrapped
// accessor, which allows them to be easily composed at the call site,
// without requiring any memory allocation.

// flow tags
class input_flow
{
};
class output_flow
{
};
class inout_flow
{
};

// flow_can_read<Flow>::value yields a compile-time boolean indicating whether
// or not the given flow supports reading.
template<class Flow>
class flow_can_read
{
};
template<>
class flow_can_read<input_flow> : std::true_type
{
};
template<>
class flow_can_read<output_flow> : std::false_type
{
};
template<>
class flow_can_read<inout_flow> : std::true_type
{
};
// convenience wrapper
template<class Flow>
inline constexpr bool flow_can_read_v = flow_can_read<Flow>::value;

// flow_can_write<Flow>::value yields a compile-time boolean indicating whether
// or not the given flow supports writing.
template<class Flow>
class flow_can_write
{
};
template<>
class flow_can_write<input_flow> : std::false_type
{
};
template<>
class flow_can_write<output_flow> : std::true_type
{
};
template<>
class flow_can_write<inout_flow> : std::true_type
{
};
// convenience wrapper
template<class Flow>
inline constexpr bool flow_can_write_v = flow_can_write<Flow>::value;

// untyped_signal_base defines functionality common to all signals, irrespective
// of the type of the value that the signal carries.
struct untyped_signal_base
{
    // Can the signal currently be read from?
    virtual bool
    is_readable() const = 0;

    // A signal must supply an ID which uniquely identifies its value.
    // The ID is required to be valid if is_readable() returns true.
    // (It may be valid even if is_readable() returns false, which would mean
    // that the signal can identify its value but doesn't know it yet.)
    // The returned ID reference is only valid as long as the signal itself is
    // valid.
    virtual id_interface const&
    id() const = 0;

    // Can the signal currently be written to?
    virtual bool
    is_writable() const = 0;
};
template<class Value, class Flow>
struct signal_interface : untyped_signal_base
{
    typedef Value value_type;

    // Read the signal's value. The reference returned here is only guaranteed
    // to be valid as long as the accessor itself is valid.
    virtual Value const&
    read() const = 0;

    // Write the signal's value.
    virtual void
    write(T const& value) const = 0;
};

// Read a signal's value.
template<class Value, class Flow>
std::enable_if_t<flow_can_read_v<Flow>, Value>
read_signal(signal

} // namespace alia
