#ifndef ALIA_SIGNALS_BASIC_HPP
#define ALIA_SIGNALS_BASIC_HPP

#include <alia/signals/core.hpp>
#include <alia/signals/utilities.hpp>

// This file defines various utilities for constructing basic signals.

namespace alia {

// empty<Value>() gives a signal that never has a value.
template<class Value>
struct empty_signal
    : signal<empty_signal<Value>, Value, readable_duplex_signal>
{
    empty_signal()
    {
    }
    id_interface const&
    value_id() const override
    {
        return null_id;
    }
    bool
    has_value() const override
    {
        return false;
    }
    // Since this never has a value, read() should never be called.
    // LCOV_EXCL_START
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnull-dereference"
#endif
    Value const&
    read() const override
    {
        throw nullptr;
    }
    Value
    movable_value() const override
    {
        throw nullptr;
    }
#ifdef __clang__
#pragma clang diagnostic pop
#endif
    // LCOV_EXCL_STOP
    bool
    ready_to_write() const override
    {
        return false;
    }
    // Since this is never ready to write, write() should never be called.
    // LCOV_EXCL_START
    void write(Value) const override
    {
    }
    // LCOV_EXCL_STOP
};
template<class Value>
empty_signal<Value>
empty()
{
    return empty_signal<Value>();
}

// value(v) creates a read-only signal that carries the value v.
template<class Value>
struct value_signal
    : regular_signal<value_signal<Value>, Value, movable_read_only_signal>
{
    explicit value_signal(Value v) : v_(std::move(v))
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    Value const&
    read() const override
    {
        return v_;
    }
    Value
    movable_value() const override
    {
        Value movable = std::move(v_);
        return movable;
    }

 private:
    mutable Value v_;
};
template<class Value>
value_signal<Value>
value(Value v)
{
    return value_signal<Value>(std::move(v));
}

// This is a special overload of value() for C-style string literals.
struct string_literal_signal
    : lazy_signal<string_literal_signal, std::string, read_only_signal>
{
    string_literal_signal(char const* x) : text_(x)
    {
    }
    id_interface const&
    value_id() const override
    {
        id_ = make_id(text_);
        return id_;
    }
    bool
    has_value() const override
    {
        return true;
    }
    std::string
    movable_value() const override
    {
        return std::string(text_);
    }

 private:
    char const* text_;
    mutable simple_id<char const*> id_;
};
inline string_literal_signal
value(char const* text)
{
    return string_literal_signal(text);
}

// literal operators
namespace literals {
inline string_literal_signal operator"" _a(char const* s, size_t)
{
    return string_literal_signal(s);
}
} // namespace literals

// direct(x), where x is a non-const reference, creates a duplex signal that
// directly exposes the value of x.
template<class Value>
struct direct_signal
    : regular_signal<direct_signal<Value>, Value, copyable_duplex_signal>
{
    explicit direct_signal(Value* v) : v_(v)
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    Value const&
    read() const override
    {
        return *v_;
    }
    Value
    movable_value() const override
    {
        Value movable = std::move(*v_);
        return movable;
    }
    bool
    ready_to_write() const override
    {
        return true;
    }
    void
    write(Value value) const override
    {
        *v_ = std::move(value);
    }

 private:
    Value* v_;
};
template<class Value>
direct_signal<Value>
direct(Value& x)
{
    return direct_signal<Value>(&x);
}

// direct(x), where x is a const reference, creates a read-only signal that
// directly exposes the value of x.
template<class Value>
struct direct_const_signal
    : regular_signal<direct_const_signal<Value>, Value, read_only_signal>
{
    explicit direct_const_signal(Value const* v) : v_(v)
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    Value const&
    read() const override
    {
        return *v_;
    }

 private:
    Value const* v_;
};
template<class Value>
direct_const_signal<Value>
direct(Value const& x)
{
    return direct_const_signal<Value>(&x);
}

} // namespace alia

#endif
