#ifndef ALIA_SIGNALS_CORE_HPP
#define ALIA_SIGNALS_CORE_HPP

#include <alia/common.hpp>
#include <alia/id.hpp>

// This file defines the core types and functions of the signals module.

namespace alia {

// Signals are passed by const reference into UI functions.
// They're typically created directly at the call site as function arguments
// and are only valid for the life of the function call.
// Signals wrappers are templated and store copies of the actual wrapped
// signal, which allows them to be easily composed at the call site,
// without requiring any memory allocation.

// direction tags
struct read_only_signal
{
};
struct write_only_signal
{
};
struct duplex_signal
{
};

// signal_direction_is_compatible<Expected,Actual>::value yields a compile-time
// boolean indicating whether or not a signal with :Actual direction can be used
// in a context expecting :Expected direction.
// In the general case, the signals are not compatible.
template<class Expected, class Actual>
struct signal_direction_is_compatible : std::false_type
{
};
// If the directions are the same, this is trivially true.
template<class Same>
struct signal_direction_is_compatible<Same, Same> : std::true_type
{
};
// A duplex signal can work as anything.
template<class Expected>
struct signal_direction_is_compatible<Expected, duplex_signal> : std::true_type
{
};
// Resolve ambiguity.
template<>
struct signal_direction_is_compatible<duplex_signal, duplex_signal>
    : std::true_type
{
};

// signal_direction_intersection<A,B>::type, where A and B are signal
// directions, yields a direction that only has the capabilities that are common
// to both A and B.
template<class A, class B>
struct signal_direction_intersection
{
};
// If the directions are the same, this is trivial.
template<class Same>
struct signal_direction_intersection<Same, Same>
{
    typedef Same type;
};
// A duplex signal has both capabilities.
template<class A>
struct signal_direction_intersection<A, duplex_signal>
{
    typedef A type;
};
template<class B>
struct signal_direction_intersection<duplex_signal, B>
{
    typedef B type;
};
// Resolve ambiguity.
template<>
struct signal_direction_intersection<duplex_signal, duplex_signal>
{
    typedef duplex_signal type;
};

// signal_direction_union<A,B>::type, where A and B are signal directions,
// yields a direction that has the union of the capabilities of A and B.
template<class A, class B>
struct signal_direction_union;
// If the directions are the same, this is trivial.
template<class Same>
struct signal_direction_union<Same, Same>
{
    typedef Same type;
};
template<class A, class B>
struct signal_direction_union
{
    // All other combinations yield duplex signals.
    typedef duplex_signal type;
};

// untyped_signal_base defines functionality common to all signals, irrespective
// of the type of the value that the signal carries.
struct untyped_signal_base
{
    // Does the signal currently have a value?
    virtual bool
    has_value() const = 0;

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
    value_id() const = 0;

    // Is the signal currently ready to write?
    virtual bool
    ready_to_write() const = 0;

    // WARNING: EXPERIMENTAL VALIDATION STUFF FOLLOWS...

    // Handle a validation error.
    //
    // This is called when there's an attempt to write to the signal and a
    // validation_error is thrown. (The argument is the error.)
    //
    // The return value should be true iff the validation error was handled.
    //
    virtual bool invalidate(std::exception_ptr) const
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
    read() const = 0;

    // Write the signal's value.
    virtual void
    write(Value value) const = 0;
};

template<class Derived, class Value, class Direction>
struct signal_base : signal_interface<Value>
{
    typedef Direction direction_tag;

    template<class Index>
    auto
    operator[](Index index) const;
};

template<class Derived, class Value, class Direction>
struct signal : signal_base<Derived, Value, Direction>
{
};

template<class Derived, class Value>
struct signal<Derived, Value, read_only_signal>
    : signal_base<Derived, Value, read_only_signal>
{
    // These must be defined to satisfy the interface requirements, but they
    // obviously won't be used on a read-only signal.
    // LCOV_EXCL_START
    bool
    ready_to_write() const
    {
        return false;
    }
    void write(Value) const
    {
    }
    // LCOV_EXCL_STOP
};

template<class Derived, class Value>
struct signal<Derived, Value, write_only_signal>
    : signal_base<Derived, Value, write_only_signal>
{
    // These must be defined to satisfy the interface requirements, but they
    // obviously won't be used on a write-only signal.
    // LCOV_EXCL_START
    id_interface const&
    value_id() const
    {
        return null_id;
    }
    bool
    has_value() const
    {
        return false;
    }
    Value const&
    read() const
    {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnull-dereference"
#endif
        return *(Value const*) nullptr;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
    }
    // LCOV_EXCL_STOP
};

// signal_ref is a reference to a signal that acts as a signal itself.
template<class Value, class Direction>
struct signal_ref : signal<signal_ref<Value, Direction>, Value, Direction>
{
    // Construct from any signal with compatible direction.
    template<class OtherSignal, class OtherDirection>
    signal_ref(
        signal<OtherSignal, Value, OtherDirection> const& signal,
        std::enable_if_t<
            signal_direction_is_compatible<Direction, OtherDirection>::value,
            int> = 0)
        : ref_(&signal)
    {
    }
    // Construct from another signal_ref. - This is meant to prevent unnecessary
    // layers of indirection.
    signal_ref(signal_ref<Value, Direction> const& other) : ref_(other.ref_)
    {
    }

    // implementation of signal_interface...

    bool
    has_value() const
    {
        return ref_->has_value();
    }
    Value const&
    read() const
    {
        return ref_->read();
    }
    id_interface const&
    value_id() const
    {
        return ref_->value_id();
    }
    bool
    ready_to_write() const
    {
        return ref_->ready_to_write();
    }
    void
    write(Value value) const
    {
        ref_->write(std::move(value));
    }
    bool
    invalidate(std::exception_ptr error) const
    {
        return ref_->invalidate(error);
    }
    bool
    is_invalidated() const
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
struct signal_is_readable : signal_direction_is_compatible<
                                read_only_signal,
                                typename Signal::direction_tag>
{
};

// is_readable_signal_type<T>::value yields a compile-time boolean indicating
// whether or not T is an alia signal that supports reading.
template<class T>
struct is_readable_signal_type : std::conditional_t<
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
struct signal_is_writable : signal_direction_is_compatible<
                                write_only_signal,
                                typename Signal::direction_tag>
{
};

// is_writable_signal_type<T>::value yields a compile-time boolean indicating
// whether or not T is an alia signal that supports writing.
template<class T>
struct is_writable_signal_type : std::conditional_t<
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
std::enable_if_t<signal_is_writable<Signal>::value>
write_signal(Signal const& signal, Value value)
{
    if (signal.ready_to_write())
    {
        try
        {
            signal.write(std::move(value));
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
}

// signal_is_duplex<Signal>::value yields a compile-time boolean indicating
// whether or not the given signal type supports both reading and writing.
template<class Signal>
struct signal_is_duplex : signal_direction_is_compatible<
                              duplex_signal,
                              typename Signal::direction_tag>
{
};

// is_duplex_signal_type<T>::value yields a compile-time boolean indicating
// whether or not T is an alia signal that supports both reading and writing.
template<class T>
struct is_duplex_signal_type : std::conditional_t<
                                   is_signal_type<T>::value,
                                   signal_is_duplex<T>,
                                   std::false_type>
{
};

} // namespace alia

#endif
