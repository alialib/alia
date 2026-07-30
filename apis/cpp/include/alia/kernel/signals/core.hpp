#pragma once

#include <alia/kernel/id.hpp>

#include <cassert>
#include <concepts>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// This file defines the core types and functions of the signals module.

namespace alia {

// Signals are passed by const reference into UI functions.
// They're typically created directly at the call site as function arguments
// and are only valid for the life of the function call.
// Signals wrappers are templated and store copies of the actual wrapped
// signal, which allows them to be easily composed at the call site,
// without requiring any memory allocation.

// The following enumerate the possible levels of capabilities that signals can
// have with respect to reading values (from the signal). The bit flags
// indicate subtyping relationships.

// The signal has no reading capabilities.
constexpr unsigned signal_unreadable = 0b0000;
// The signal can return a const reference to its value.
constexpr unsigned signal_readable = 0b0001;
// The signal is capable of moving out its value, but there may be side
// effects, so it requires explicit activation.
constexpr unsigned signal_movable = 0b0011;
// The signal can move out its value.
constexpr unsigned signal_move_activated = 0b0111;

// The following are the same, but for writing.
constexpr unsigned signal_unwritable = 0b00;
constexpr unsigned signal_writable = 0b01;
constexpr unsigned signal_clearable = 0b11;

// combined capabilities
template<unsigned Reading, unsigned Writing>
struct signal_capabilities
{
    static constexpr unsigned reading = Reading;
    static constexpr unsigned writing = Writing;
};

// Signals in Alia are grouped into three basic roles:
// - view: a read-only signal
// - binding: a bidirectional signal
// - sink: a write-only signal (much less common than the other two)

// The following allow you to construct capability packs for the various roles
// at varying levels of capability.

template<unsigned Reading>
using view_caps = signal_capabilities<Reading, signal_unwritable>;

template<unsigned Writing>
using sink_caps = signal_capabilities<signal_unreadable, Writing>;

template<unsigned Reading, unsigned Writing = signal_writable>
using binding_caps = signal_capabilities<Reading, Writing>;

// signal_capability_level_is_compatible<Expected,Actual> is true iff a
// signal with `Actual` capability level can be used in a context expecting
// `Expected` capability level.
template<unsigned Expected, unsigned Actual>
constexpr bool signal_capability_level_is_compatible
    = (Expected & Actual) == Expected;

// signal_capabilities_compatible<Expected,Actual> is true iff a signal with
// `Actual` capabilities can be used in a context expecting `Expected`
// capabilities.
template<class Expected, class Actual>
constexpr bool signal_capabilities_compatible
    = signal_capability_level_is_compatible<Expected::reading, Actual::reading>
   && signal_capability_level_is_compatible<
          Expected::writing,
          Actual::writing>;

// signal_capability_level_intersection<A,B> is the capability level that is
// common to both `A` and `B`.
template<unsigned A, unsigned B>
constexpr unsigned signal_capability_level_intersection = A & B;

// signal_capabilities_intersection<A,B> is the intersection of the two
// capability packs `A` and `B`, i.e. the set of capabilities that are
// supported by both `A` and `B`.
template<class A, class B>
using signal_capabilities_intersection = signal_capabilities<
    signal_capability_level_intersection<A::reading, B::reading>,
    signal_capability_level_intersection<A::writing, B::writing>>;

// signal_capability_level_union<A,B> is the capability level that is the
// union of `A` and `B`, i.e. the set of capabilities that are supported by
// either `A` or `B`.
template<unsigned A, unsigned B>
constexpr unsigned signal_capability_level_union = A | B;

// signal_capabilities_union<A,B> is the union of the two capability packs
// `A` and `B`, i.e. the set of capabilities that are supported by either
// `A` or `B`.
template<class A, class B>
using signal_capabilities_union = signal_capabilities<
    signal_capability_level_union<A::reading, B::reading>,
    signal_capability_level_union<A::writing, B::writing>>;

// `untyped_signal_base` defines functionality common to all signals,
// irrespective of the type of the value that the signal carries.
struct untyped_signal_base
{
    // virtual destructor - Signals really aren't meant to be stored by a
    // pointer-to-base, so in theory this shouldn't be necessary, but it seems
    // there's no way to avoid warnings without.
    virtual ~untyped_signal_base()
    {
    }

    // Does the signal currently have a value?
    virtual bool
    has_value() const = 0;

    // A signal must supply an ID that uniquely identifies its value.
    //
    // The ID is required to be valid if has_value() returns true.
    // (It may be valid even if has_value() returns false, which would mean
    // that the signal can identify its value but doesn't know it yet.)
    //
    // The returned ID is a transient view and is only guaranteed to be valid
    // as long as the signal itself is valid.
    //
    virtual id_view
    value_id() const = 0;

    // Is the signal currently ready to write?
    virtual bool
    ready_to_write() const = 0;

    // Clear the signal.
    virtual void
    clear() const = 0;

    // WARNING: EXPERIMENTAL VALIDATION STUFF FOLLOWS...

    // Handle a validation error.
    //
    // This is called when there's an attempt to write to the signal and a
    // validation_error is thrown. (The argument is the error.)
    //
    // The return value should be true iff the validation error was handled.
    //
    virtual bool
    invalidate(std::exception_ptr) const
    {
        return false;
    }

    // Is this signal currently invalidated?
    virtual bool
    is_invalidated() const
    {
        return false;
    }
};

template<class Value>
struct signal_interface : untyped_signal_base
{
    using value_type = Value;

    // Read the signal's value. The reference returned here is only guaranteed
    // to be valid as long as the signal object itself is valid.
    virtual Value const&
    read() const = 0;

    // Move out the signal's value.
    // This is expected to be implemented by movable signals.
    virtual Value
    move_out() const = 0;

    // Get a reference to the signal's value that the caller can manipulate
    // as it pleases.
    // This is expected to be implemented by movable signals.
    virtual Value&
    destructive_ref() const = 0;

    // Write the signal's value.
    // The signal can *optionally* return the new value ID of the signal (after
    // taking on the value that was written in by this call). If this is
    // impractical, it can just return null_id() instead.
    virtual id_view
    write(Value value) const = 0;
};

template<class Derived, class Value, class Capabilities>
struct signal_base : signal_interface<Value>
{
    using capabilities = Capabilities;
};

template<class Derived, class Value, class Capabilities>
struct signal : signal_base<Derived, Value, Capabilities>
{
};

// The following implement the various unused functions that are required by
// `signal_interface` but won't be used because of the capabilities of the
// signal...

// LCOV_EXCL_START

#define ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()                           \
    void clear() const override                                               \
    {                                                                         \
    }

#define ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)                      \
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()                               \
    bool ready_to_write() const override                                      \
    {                                                                         \
        return false;                                                         \
    }                                                                         \
    id_view write(Value) const override                                       \
    {                                                                         \
        return null_id();                                                     \
    }

#define ALIA_DEFINE_UNUSED_SIGNAL_MOVE_INTERFACE(Value)                       \
    Value move_out() const override                                           \
    {                                                                         \
        throw nullptr;                                                        \
    }                                                                         \
    Value& destructive_ref() const override                                   \
    {                                                                         \
        throw nullptr;                                                        \
    }

#define ALIA_DEFINE_UNUSED_SIGNAL_READ_INTERFACE(Value)                       \
    ALIA_DEFINE_UNUSED_SIGNAL_MOVE_INTERFACE(Value)                           \
    id_view value_id() const override                                         \
    {                                                                         \
        return null_id();                                                     \
    }                                                                         \
    bool has_value() const override                                           \
    {                                                                         \
        return false;                                                         \
    }                                                                         \
    Value const& read() const override                                        \
    {                                                                         \
        throw nullptr;                                                        \
    }

template<class Derived, class Value>
struct signal<Derived, Value, view_caps<signal_readable>>
    : signal_base<Derived, Value, view_caps<signal_readable>>
{
    ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)
    ALIA_DEFINE_UNUSED_SIGNAL_MOVE_INTERFACE(Value)
};

template<class Derived, class Value>
struct signal<Derived, Value, view_caps<signal_move_activated>>
    : signal_base<Derived, Value, view_caps<signal_move_activated>>
{
    ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)
};

template<class Derived, class Value>
struct signal<Derived, Value, view_caps<signal_movable>>
    : signal_base<Derived, Value, view_caps<signal_movable>>
{
    ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)
};

template<class Derived, class Value>
struct signal<Derived, Value, sink_caps<signal_writable>>
    : signal_base<Derived, Value, sink_caps<signal_writable>>
{
    ALIA_DEFINE_UNUSED_SIGNAL_READ_INTERFACE(Value)
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value>
struct signal<Derived, Value, binding_caps<signal_readable>>
    : signal_base<Derived, Value, binding_caps<signal_readable>>
{
    ALIA_DEFINE_UNUSED_SIGNAL_MOVE_INTERFACE(Value)
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value>
struct signal<Derived, Value, binding_caps<signal_movable>>
    : signal_base<Derived, Value, binding_caps<signal_movable>>
{
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value>
struct signal<Derived, Value, binding_caps<signal_move_activated>>
    : signal_base<Derived, Value, binding_caps<signal_move_activated>>
{
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value>
struct signal<Derived, Value, binding_caps<signal_readable, signal_clearable>>
    : signal_base<
          Derived,
          Value,
          binding_caps<signal_readable, signal_clearable>>
{
    ALIA_DEFINE_UNUSED_SIGNAL_MOVE_INTERFACE(Value)
};

// LCOV_EXCL_STOP

// signal_ref is a reference to a signal that acts as a signal itself.
template<class Value, class Capabilities>
struct signal_ref
    : signal<signal_ref<Value, Capabilities>, Value, Capabilities>
{
    template<class V, class C>
    friend struct signal_ref;

    // Construct from any signal with compatible capabilities.
    template<class OtherSignal, class OtherCapabilities>
        requires signal_capabilities_compatible<
            Capabilities,
            OtherCapabilities>
    signal_ref(signal<OtherSignal, Value, OtherCapabilities> const& signal)
        : ref_(&signal)
    {
    }
    // Construct from another signal_ref. - This is meant to prevent
    // unnecessary layers of indirection.
    template<class OtherCapabilities>
        requires signal_capabilities_compatible<
            Capabilities,
            OtherCapabilities>
    signal_ref(signal_ref<Value, OtherCapabilities> const& other)
        : ref_(other.ref_)
    {
    }

    // implementation of signal_interface...

    bool
    has_value() const override
    {
        return ref_->has_value();
    }
    Value const&
    read() const override
    {
        return ref_->read();
    }
    Value
    move_out() const override
    {
        return ref_->move_out();
    }
    Value&
    destructive_ref() const override
    {
        return ref_->destructive_ref();
    }
    id_view
    value_id() const override
    {
        return ref_->value_id();
    }
    bool
    ready_to_write() const override
    {
        return ref_->ready_to_write();
    }
    id_view
    write(Value value) const override
    {
        return ref_->write(std::move(value));
    }
    bool
    invalidate(std::exception_ptr error) const override
    {
        return ref_->invalidate(error);
    }
    bool
    is_invalidated() const override
    {
        return ref_->is_invalidated();
    }

 private:
    signal_interface<Value> const* ref_;
};

// `signal_type<T>` is true iff `T` is an Alia signal type.
template<class T>
concept signal_type = std::is_base_of_v<untyped_signal_base, T>;

// `signal_with<Signal, Caps>` is true iff `Signal` is compatible with the
// capability pack `Caps`.
template<class Signal, class Caps>
concept signal_with
    = signal_type<Signal>
   && signal_capabilities_compatible<Caps, typename Signal::capabilities>;

// `signal_of<Signal, Value, Caps>` is true iff `Signal` carries `Value` and is
// compatible with `Caps`.
template<class Signal, class Value, class Caps>
concept signal_of = signal_with<Signal, Caps>
                 && std::same_as<typename Signal::value_type, Value>;

// Define the erased typedef and matching concepts for a standard signal role.
#define ALIA_DEFINE_SIGNAL_SUGAR(name, Caps)                                  \
    template<class Value>                                                     \
    using name = signal_ref<Value, Caps>;                                     \
    template<class Signal>                                                    \
    concept name##_signal = signal_with<Signal, Caps>;                        \
    template<class Signal, class Value>                                       \
    concept name##_of = signal_of<Signal, Value, Caps>;

// Define the types and concepts for the three main signal roles.
ALIA_DEFINE_SIGNAL_SUGAR(view, view_caps<signal_readable>)
ALIA_DEFINE_SIGNAL_SUGAR(sink, sink_caps<signal_writable>)
ALIA_DEFINE_SIGNAL_SUGAR(binding, binding_caps<signal_movable>)

#undef ALIA_DEFINE_SIGNAL_SUGAR

// Does `signal` currently have a value?
// Unlike calling signal.has_value() directly, this will generate a
// compile-time error if the signal's type doesn't support reading.
template<view_signal Signal>
bool
signal_has_value(Signal const& signal)
{
    return signal.has_value();
}

// Read a signal's value.
// Unlike calling signal.read() directly, this will generate a compile-time
// error if the signal's type doesn't support reading and a run-time error if
// the signal doesn't currently have a value.
template<view_signal Signal>
Signal::value_type const&
read_signal(Signal const& signal)
{
    assert(signal.has_value());
    return signal.read();
}

// When a value is written to a signal, the signal is allowed to throw a
// validation_error if the value isn't acceptable.
struct validation_error : std::runtime_error
{
    validation_error(std::string const& message) : std::runtime_error(message)
    {
    }
    ~validation_error() noexcept(true)
    {
    }
};

// Is `signal` ready to write?
// Unlike calling signal.ready_to_write() directly, this will generate a
// compile-time error if the signal's type doesn't support writing.
template<sink_signal Signal>
bool
signal_ready_to_write(Signal const& signal)
{
    return signal.ready_to_write();
}

// Write a signal's value.
// Unlike calling signal.write() directly, this will generate a compile-time
// error if the signal's type doesn't support writing.
// Note that if the signal isn't ready to write, this is a no op.
template<sink_signal Signal, class Value>
id_view
write_signal(Signal const& signal, Value value)
{
    if (signal.ready_to_write())
    {
        try
        {
            return signal.write(std::move(value));
        }
        catch (validation_error&)
        {
            // EXPERIMENTAL VALIDATION LOGIC: Try to let the signal handle the
            // validation error (at some level). If it can't, rethrow the
            // exception.
            auto e = std::current_exception();
            if (!signal.invalidate(e))
                std::rethrow_exception(e);
        }
    }
    return null_id();
}

// Move out a signal's value.
template<signal_with<view_caps<signal_move_activated>> Signal>
Signal::value_type
move_signal(Signal const& signal)
{
    assert(signal.has_value());
    return signal.move_out();
}

// Forward along a signal's value.
// This will move out the value if movement is activated or return a reference
// otherwise.
template<signal_with<view_caps<signal_move_activated>> Signal>
Signal::value_type
forward_signal(Signal const& signal)
{
    assert(signal.has_value());
    return signal.move_out();
}
template<view_signal Signal>
    requires(!signal_with<Signal, view_caps<signal_move_activated>>)
Signal::value_type const&
forward_signal(Signal const& signal)
{
    assert(signal.has_value());
    return signal.read();
}

// Clear a signal's value.
// Unlike calling signal.clear() directly, this will generate a compile-time
// error if the signal's type doesn't support clearing.
// Note that if the signal isn't ready to write, this is a no op.
template<signal_with<sink_caps<signal_clearable>> Signal>
void
clear_signal(Signal const& signal)
{
    if (signal.ready_to_write())
        signal.clear();
}

} // namespace alia
