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
// accessor, which allows them to be easily composed at the call site,
// without requiring any memory allocation.

// direction tags
struct read_only_signal
{
};
struct write_only_signal
{
};
struct bidirectional_signal
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
// A bidirectional signal can work as anything.
template<class Expected>
struct signal_direction_is_compatible<Expected, bidirectional_signal>
    : std::true_type
{
};
// Resolve ambiguity.
template<>
struct signal_direction_is_compatible<
    bidirectional_signal,
    bidirectional_signal> : std::true_type
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
// A bidirectional signal has both capabilities.
template<class A>
struct signal_direction_intersection<A, bidirectional_signal>
{
    typedef A type;
};
template<class B>
struct signal_direction_intersection<bidirectional_signal, B>
{
    typedef B type;
};
// Resolve ambiguity.
template<>
struct signal_direction_intersection<bidirectional_signal, bidirectional_signal>
{
    typedef bidirectional_signal type;
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
    // All other combinations yield bidirectional signals.
    typedef bidirectional_signal type;
};

// untyped_signal_base defines functionality common to all signals, irrespective
// of the type of the value that the signal carries.
struct untyped_signal_base
{
    // Can the signal currently be read from?
    virtual bool
    is_readable() const = 0;

    // A signal must supply an ID that uniquely identifies its value.
    // The ID is required to be valid if is_readable() returns true.
    // (It may be valid even if is_readable() returns false, which would mean
    // that the signal can identify its value but doesn't know it yet.)
    // The returned ID reference is only valid as long as the signal itself is
    // valid.
    virtual id_interface const&
    value_id() const = 0;

    // Can the signal currently be written to?
    virtual bool
    is_writable() const = 0;
};

template<class Value>
struct signal_interface : untyped_signal_base
{
    typedef Value value_type;

    // Read the signal's value. The reference returned here is only guaranteed
    // to be valid as long as the accessor itself is valid.
    virtual Value const&
    read() const = 0;

    // Write the signal's value.
    virtual void
    write(Value const& value) const = 0;
};

template<class Value, class Direction>
struct signal : signal_interface<Value>
{
    typedef Direction direction_tag;
};

template<class Value>
struct signal<Value, read_only_signal> : signal_interface<Value>
{
    typedef read_only_signal direction_tag;

    // These must be defined to satisfy the interface requirements, but they
    // obviously won't be used on a read-only signal.
    // LCOV_EXCL_START
    bool
    is_writable() const
    {
        return false;
    }
    void
    write(Value const& value) const
    {
    }
    // LCOV_EXCL_STOP
};

template<class Value>
struct signal<Value, write_only_signal> : signal_interface<Value>
{
    typedef write_only_signal direction_tag;

    // These must be defined to satisfy the interface requirements, but they
    // obviously won't be used on a write-only signal.
    // LCOV_EXCL_START
    id_interface const&
    value_id() const
    {
        return no_id;
    }
    bool
    is_readable() const
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
struct signal_ref : signal<Value, Direction>
{
    // Construct from any signal with compatible direction.
    template<class OtherDirection>
    signal_ref(
        signal<Value, OtherDirection> const& signal,
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
    is_readable() const
    {
        return ref_->is_readable();
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
    is_writable() const
    {
        return ref_->is_writable();
    }
    void
    write(Value const& value) const
    {
        ref_->write(value);
    }

 private:
    signal_interface<Value> const* ref_;
};
// Construct a signal_ref for :signal.
template<class Value, class Direction>
signal_ref<Value, Direction>
ref(signal_ref<Value, Direction> const& signal)
{
    return signal_ref<Value, Direction>(signal);
}

// input<Value> denotes a reference to a readable signal carrying values of
// type :Value.
template<class Value>
using input = signal_ref<Value, read_only_signal>;

// output<Value> denotes a reference to a writable signal carrying values of
// type :Value.
template<class Value>
using output = signal_ref<Value, write_only_signal>;

// bidirectional<Value> denotes a reference to a bidirectional signal carrying
// values of type :Value.
template<class Value>
using bidirectional = signal_ref<Value, bidirectional_signal>;

// is_signal_type<T>::value yields a compile-time boolean indicating whether or
// not T is an alia signal type.
template<class T>
struct is_signal_type : std::is_base_of<untyped_signal_base, T>
{
};

// signal_can_read<Signal>::value yields a compile-time boolean indicating
// whether or not the given signal type supports reading.
template<class Signal>
struct signal_can_read : signal_direction_is_compatible<
                             read_only_signal,
                             typename Signal::direction_tag>
{
};

// is_readable_signal_type<T>::value yields a compile-time boolean indicating
// whether or not T is an alia signal that supports reading.
template<class T>
struct is_readable_signal_type : std::conditional_t<
                                     is_signal_type<T>::value,
                                     signal_can_read<T>,
                                     std::false_type>
{
};

// Is :signal currently readable?
// Unlike calling signal.is_readable() directly, this will generate a
// compile-time error if the signal's type doesn't support reading.
template<class Signal>
std::enable_if_t<signal_can_read<Signal>::value, bool>
signal_is_readable(Signal const& signal)
{
    return signal.is_readable();
}

// Read a signal's value.
// Unlike calling signal.read() directly, this will generate a compile-time
// error if the signal's type doesn't support reading and a run-time error if
// the signal isn't currently readable.
template<class Signal>
std::enable_if_t<
    signal_can_read<Signal>::value,
    typename Signal::value_type const&>
read_signal(Signal const& signal)
{
    assert(signal.is_readable());
    return signal.read();
}

// signal_can_write<Signal>::value yields a compile-time boolean indicating
// whether or not the given signal type supports writing.
template<class Signal>
struct signal_can_write : signal_direction_is_compatible<
                              write_only_signal,
                              typename Signal::direction_tag>
{
};

// is_writable_signal_type<T>::value yields a compile-time boolean indicating
// whether or not T is an alia signal that supports writing.
template<class T>
struct is_writable_signal_type : std::conditional_t<
                                     is_signal_type<T>::value,
                                     signal_can_write<T>,
                                     std::false_type>
{
};

// Is :signal currently writable?
// Unlike calling signal.is_writable() directly, this will generate a
// compile-time error if the signal's type doesn't support writing.
template<class Signal>
std::enable_if_t<signal_can_write<Signal>::value, bool>
signal_is_writable(Signal const& signal)
{
    return signal.is_writable();
}

// Write a signal's value.
// Unlike calling signal.write() directly, this will generate a compile-time
// error if the signal's type doesn't support writing and a run-time error if
// the signal isn't currently writable.
template<class Signal, class Value>
std::enable_if_t<signal_can_write<Signal>::value>
write_signal(Signal const& signal, Value const& value)
{
    assert(signal.is_writable());
    signal.write(value);
}

} // namespace alia

#endif
