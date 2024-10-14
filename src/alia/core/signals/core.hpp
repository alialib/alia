#ifndef ALIA_CORE_SIGNALS_CORE_HPP
#define ALIA_CORE_SIGNALS_CORE_HPP

#include <alia/core/common.hpp>
#include <alia/core/id.hpp>

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

// useful signal capability combinations
typedef signal_capabilities<signal_readable, signal_unwritable>
    read_only_signal;
typedef signal_capabilities<signal_movable, signal_unwritable>
    movable_read_only_signal;
typedef signal_capabilities<signal_move_activated, signal_unwritable>
    move_activated_signal;
typedef signal_capabilities<signal_unreadable, signal_writable>
    write_only_signal;
typedef signal_capabilities<signal_readable, signal_writable>
    readable_duplex_signal;
typedef signal_capabilities<signal_movable, signal_writable>
    movable_duplex_signal;
typedef signal_capabilities<signal_move_activated, signal_writable>
    move_activated_duplex_signal;
typedef movable_duplex_signal duplex_signal;
typedef signal_capabilities<signal_readable, signal_clearable>
    clearable_signal;
typedef signal_capabilities<signal_move_activated, signal_clearable>
    move_activated_clearable_signal;

// signal_capability_level_is_compatible<Expected,Actual>::value yields a
// compile-time boolean indicating whether or not a signal with :Actual
// capability level can be used in a context expecting :Expected capability
// level.
template<unsigned Expected, unsigned Actual>
struct signal_capability_level_is_compatible
{
    static constexpr bool value = (Expected & Actual) == Expected;
};

// signal_capabilities_compatible<Expected,Actual>::value yields a compile-time
// boolean indicating whether or not a signal with :Actual capabilities can be
// used in a context expecting :Expected capabilities.
template<class Expected, class Actual>
struct signal_capabilities_compatible
    : std::conditional_t<
          signal_capability_level_is_compatible<
              Expected::reading,
              Actual::reading>::value,
          signal_capability_level_is_compatible<
              Expected::writing,
              Actual::writing>,
          std::false_type>
{
};

// signal_capability_level_intersection<A,B> yields the type representing
// the intersection of the capability levels :A and :B.
template<unsigned A, unsigned B>
struct signal_capability_level_intersection
{
    static constexpr unsigned value = A & B;
};

// signal_capabilities_intersection<A,B>::type, where A and B are signal
// capability tags, yields a tag that only has the capabilities that are common
// to both A and B.
template<class A, class B>
struct signal_capabilities_intersection
{
    typedef signal_capabilities<
        signal_capability_level_intersection<A::reading, B::reading>::value,
        signal_capability_level_intersection<A::writing, B::writing>::value>
        type;
};

// signal_level_capability_union<A,B> yields the type representing the
// union of the capability levels :A and :B.
template<unsigned A, unsigned B>
struct signal_capability_level_union
{
    static constexpr unsigned value = A | B;
};

// signal_capabilities_union<A,B>::type, where A and B are signal capability
// tags, yields a tag that has the union of the capabilities of A and B.
template<class A, class B>
struct signal_capabilities_union
{
    typedef signal_capabilities<
        signal_capability_level_union<A::reading, B::reading>::value,
        signal_capability_level_union<A::writing, B::writing>::value>
        type;
};

// untyped_signal_base defines functionality common to all signals,
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
    has_value() const
        = 0;

    // A signal must supply an ID that uniquely identifies its value.
    //
    // The ID is required to be valid if has_value() returns true.
    // (It may be valid even if has_value() returns false, which would mean
    // that the signal can identify its value but doesn't know it yet.)
    //
    // The returned ID reference is only guaranteed to be valid as long as the
    // signal itself is valid.
    //
    virtual id_interface const&
    value_id() const
        = 0;

    // Is the signal currently ready to write?
    virtual bool
    ready_to_write() const
        = 0;

    // Clear the signal.
    virtual void
    clear() const
        = 0;

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
    typedef Value value_type;

    // Read the signal's value. The reference returned here is only guaranteed
    // to be valid as long as the signal object itself is valid.
    virtual Value const&
    read() const
        = 0;

    // Move out the signal's value.
    // This is expected to be implemented by movable signals.
    virtual Value
    move_out() const
        = 0;

    // Get a reference to the signal's value that the caller can manipulate
    // as it pleases.
    // This is expected to be implemented by movable signals.
    virtual Value&
    destructive_ref() const
        = 0;

    // Write the signal's value.
    // The signal can *optionally* return the new value ID of the signal (after
    // taking on the value that was written in by this call). If this is
    // impractical, it can just return null_id instead.
    virtual id_interface const&
    write(Value value) const
        = 0;
};

template<class Derived, class Value, class Capabilities>
struct signal_base : signal_interface<Value>
{
    typedef Capabilities capabilities;

    template<class Index>
    auto
    operator[](Index index) const;
};

template<class Derived, class Value, class Capabilities>
struct signal : signal_base<Derived, Value, Capabilities>
{
};

// The following implement the various unused functions that are required by
// the signal_interface but won't be used because of the capabilities of the
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
    id_interface const& write(Value) const override                           \
    {                                                                         \
        return null_id;                                                       \
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
    id_interface const& value_id() const override                             \
    {                                                                         \
        return null_id;                                                       \
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
struct signal<Derived, Value, read_only_signal>
    : signal_base<Derived, Value, read_only_signal>
{
    ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)
    ALIA_DEFINE_UNUSED_SIGNAL_MOVE_INTERFACE(Value)
};

template<class Derived, class Value>
struct signal<Derived, Value, move_activated_signal>
    : signal_base<Derived, Value, move_activated_signal>
{
    ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)
};

template<class Derived, class Value>
struct signal<Derived, Value, movable_read_only_signal>
    : signal_base<Derived, Value, movable_read_only_signal>
{
    ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)
};

template<class Derived, class Value>
struct signal<Derived, Value, write_only_signal>
    : signal_base<Derived, Value, write_only_signal>
{
    ALIA_DEFINE_UNUSED_SIGNAL_READ_INTERFACE(Value)
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value>
struct signal<Derived, Value, readable_duplex_signal>
    : signal_base<Derived, Value, readable_duplex_signal>
{
    ALIA_DEFINE_UNUSED_SIGNAL_MOVE_INTERFACE(Value)
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value>
struct signal<Derived, Value, movable_duplex_signal>
    : signal_base<Derived, Value, movable_duplex_signal>
{
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value>
struct signal<Derived, Value, move_activated_duplex_signal>
    : signal_base<Derived, Value, move_activated_duplex_signal>
{
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value>
struct signal<Derived, Value, clearable_signal>
    : signal_base<Derived, Value, clearable_signal>
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
    signal_ref(
        signal<OtherSignal, Value, OtherCapabilities> const& signal,
        std::enable_if_t<
            signal_capabilities_compatible<Capabilities, OtherCapabilities>::
                value,
            int>
        = 0)
        : ref_(&signal)
    {
    }
    // Construct from another signal_ref. - This is meant to prevent
    // unnecessary layers of indirection.
    template<class OtherCapabilities>
    signal_ref(
        signal_ref<Value, OtherCapabilities> const& other,
        std::enable_if_t<
            signal_capabilities_compatible<Capabilities, OtherCapabilities>::
                value,
            int>
        = 0)
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
    id_interface const&
    value_id() const override
    {
        return ref_->value_id();
    }
    bool
    ready_to_write() const override
    {
        return ref_->ready_to_write();
    }
    id_interface const&
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

// readable<Value> denotes a reference to a readable signal carrying values of
// type :Value.
template<class Value>
using readable = signal_ref<Value, read_only_signal>;

// writable<Value> denotes a reference to a writable signal carrying values of
// type :Value.
template<class Value>
using writable = signal_ref<Value, write_only_signal>;

// duplex<Value> denotes a reference to a duplex signal carrying values of type
// :Value.
template<class Value>
using duplex = signal_ref<Value, duplex_signal>;

// is_signal_type<T>::value yields a compile-time boolean indicating whether or
// not T is an alia signal type.
template<class T>
struct is_signal_type : std::is_base_of<untyped_signal_base, T>
{
};

// signal_is_readable<Signal>::value yields a compile-time boolean indicating
// whether or not the given signal type supports reading.
template<class Signal>
struct signal_is_readable
    : signal_capability_level_is_compatible<
          signal_readable,
          Signal::capabilities::reading>
{
};

// is_readable_signal_type<T>::value yields a compile-time boolean indicating
// whether or not T is an alia signal that supports reading.
template<class T>
struct is_readable_signal_type
    : std::conditional_t<
          is_signal_type<T>::value,
          signal_is_readable<T>,
          std::false_type>
{
};

// Does :signal currently have a value?
// Unlike calling signal.has_value() directly, this will generate a
// compile-time error if the signal's type doesn't support reading.
template<class Signal>
std::enable_if_t<signal_is_readable<Signal>::value, bool>
signal_has_value(Signal const& signal)
{
    return signal.has_value();
}

// Read a signal's value.
// Unlike calling signal.read() directly, this will generate a compile-time
// error if the signal's type doesn't support reading and a run-time error if
// the signal doesn't currently have a value.
template<class Signal>
std::enable_if_t<
    signal_is_readable<Signal>::value,
    typename Signal::value_type const&>
read_signal(Signal const& signal)
{
    assert(signal.has_value());
    return signal.read();
}

// When a value is written to a signal, the signal is allowed to throw a
// validation_error if the value isn't acceptable.
struct validation_error : exception
{
    validation_error(std::string const& message) : exception(message)
    {
    }
    ~validation_error() noexcept(true)
    {
    }
};

// signal_is_writable<Signal>::value yields a compile-time boolean indicating
// whether or not the given signal type supports writing.
template<class Signal>
struct signal_is_writable
    : signal_capability_level_is_compatible<
          signal_writable,
          Signal::capabilities::writing>
{
};

// is_writable_signal_type<T>::value yields a compile-time boolean indicating
// whether or not T is an alia signal that supports writing.
template<class T>
struct is_writable_signal_type
    : std::conditional_t<
          is_signal_type<T>::value,
          signal_is_writable<T>,
          std::false_type>
{
};

// Is :signal ready to write?
// Unlike calling signal.ready_to_write() directly, this will generate a
// compile-time error if the signal's type doesn't support writing.
template<class Signal>
std::enable_if_t<signal_is_writable<Signal>::value, bool>
signal_ready_to_write(Signal const& signal)
{
    return signal.ready_to_write();
}

// Write a signal's value.
// Unlike calling signal.write() directly, this will generate a compile-time
// error if the signal's type doesn't support writing.
// Note that if the signal isn't ready to write, this is a no op.
template<class Signal, class Value>
std::enable_if_t<signal_is_writable<Signal>::value, id_interface const&>
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
    return null_id;
}

// signal_is_duplex<Signal>::value yields a compile-time boolean indicating
// whether or not the given signal type supports both reading and writing.
template<class Signal>
struct signal_is_duplex
    : signal_capabilities_compatible<
          readable_duplex_signal,
          typename Signal::capabilities>
{
};

// is_duplex_signal_type<T>::value yields a compile-time boolean indicating
// whether or not T is an alia signal that supports both reading and writing.
template<class T>
struct is_duplex_signal_type
    : std::conditional_t<
          is_signal_type<T>::value,
          signal_is_duplex<T>,
          std::false_type>
{
};

// signal_is_movable<Signal>::value yields a compile-time boolean indicating
// whether or not the given signal type supports copying.
template<class Signal>
struct signal_is_movable
    : signal_capability_level_is_compatible<
          signal_movable,
          Signal::capabilities::reading>
{
};

// is_movable_signal_type<T>::value yields a compile-time boolean indicating
// whether or not T is an alia signal that supports value copying.
template<class T>
struct is_movable_signal_type
    : std::conditional_t<
          is_signal_type<T>::value,
          signal_is_movable<T>,
          std::false_type>
{
};

// signal_is_move_activated<Signal>::value yields a compile-time boolean
// indicating whether or not the given signal type allows moving.
template<class Signal>
struct signal_is_move_activated
    : signal_capability_level_is_compatible<
          signal_move_activated,
          Signal::capabilities::reading>
{
};

// is_move_activated_signal_type<T>::value yields a compile-time boolean
// indicating whether or not T is an alia signal that supports value movement.
template<class T>
struct is_move_activated_signal_type
    : std::conditional_t<
          is_signal_type<T>::value,
          signal_is_move_activated<T>,
          std::false_type>
{
};

// Move out a signal's value.
template<class Signal>
std::enable_if_t<
    signal_is_move_activated<Signal>::value,
    typename Signal::value_type>
move_signal(Signal const& signal)
{
    assert(signal.has_value());
    return signal.move_out();
}

// Forward along a signal's value.
// This will move out the value if movement is activated or return a reference
// otherwise.
template<class Signal>
std::enable_if_t<
    signal_is_move_activated<Signal>::value,
    typename Signal::value_type>
forward_signal(Signal const& signal)
{
    assert(signal.has_value());
    return signal.move_out();
}
template<class Signal>
std::enable_if_t<
    !signal_is_move_activated<Signal>::value
        && signal_is_readable<Signal>::value,
    typename Signal::value_type const&>
forward_signal(Signal const& signal)
{
    assert(signal.has_value());
    return signal.read();
}

// signal_is_clearable<Signal>::value yields a compile-time boolean indicating
// whether or not the given signal type supports clearing.
template<class Signal>
struct signal_is_clearable
    : signal_capability_level_is_compatible<
          signal_clearable,
          Signal::capabilities::writing>
{
};

// is_clearable_signal_type<T>::value yields a compile-time boolean indicating
// whether or not T is an alia signal that supports clearing.
template<class T>
struct is_clearable_signal_type
    : std::conditional_t<
          is_signal_type<T>::value,
          signal_is_clearable<T>,
          std::false_type>
{
};

// Clear a signal's value.
// Unlike calling signal.clear() directly, this will generate a compile-time
// error if the signal's type doesn't support clearing.
// Note that if the signal isn't ready to write, this is a no op.
template<class Signal>
std::enable_if_t<signal_is_clearable<Signal>::value>
clear_signal(Signal const& signal)
{
    if (signal.ready_to_write())
        signal.clear();
}

} // namespace alia

#endif
