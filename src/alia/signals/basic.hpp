#ifndef ALIA_SIGNALS_BASIC_HPP
#define ALIA_SIGNALS_BASIC_HPP

#include <alia/signals/core.hpp>
#include <alia/signals/utilities.hpp>

// This file defines various utilities for constructing basic signals.

namespace alia {

// empty<Value>() gives a signal that never has a value.
template<class Value>
struct empty_signal : signal<Value, bidirectional_signal>
{
    empty_signal()
    {
    }
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
    // Since this is never readable, read() should never be called.
    // LCOV_EXCL_START
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
    bool
    is_writable() const
    {
        return false;
    }
    // Since this is never writable, write() should never be called.
    // LCOV_EXCL_START
    void
    write(Value const& value) const
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
struct value_signal : regular_signal<Value, read_only_signal>
{
    explicit value_signal(Value const& v) : v_(v)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    Value const&
    read() const
    {
        return v_;
    }

 private:
    Value v_;
};
template<class Value>
value_signal<Value>
value(Value const& v)
{
    return value_signal<Value>(v);
}

// This is a special overload of value() for C-style string literals.
struct string_literal_signal : signal<std::string, read_only_signal>
{
    string_literal_signal(char const* x) : text_(x)
    {
    }
    id_interface const&
    value_id() const
    {
        return unit_id;
    }
    bool
    is_readable() const
    {
        return true;
    }
    std::string const&
    read() const
    {
        return lazy_reader_.read([&] { return std::string(text_); });
    }

 private:
    char const* text_;
    lazy_reader<std::string> lazy_reader_;
};
inline string_literal_signal
value(char const* text)
{
    return string_literal_signal(text);
}

// direct(x) creates a bidirectional signal that directly exposes the value of
// x.
template<class Value>
struct direct_signal : regular_signal<Value, bidirectional_signal>
{
    explicit direct_signal(Value* v) : v_(v)
    {
    }
    bool
    is_readable() const
    {
        return true;
    }
    Value const&
    read() const
    {
        return *v_;
    }
    bool
    is_writable() const
    {
        return true;
    }
    void
    write(Value const& value) const
    {
        *v_ = value;
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

} // namespace alia

#endif
