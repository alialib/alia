#pragma once

#include <alia/kernel/signals/core.hpp>
#include <alia/kernel/signals/utilities.hpp>

#include <string>
#include <type_traits>
#include <utility>

// This file defines utilities for constructing basic signals.

namespace alia {

// `empty<Value>()` gives a signal that never has a value.
template<class Value>
struct empty_signal
    : signal<
          empty_signal<Value>,
          Value,
          binding_caps<signal_move_activated, signal_clearable>,
          constant_value_tag>
{
    empty_signal()
    {
    }
    constant_value_tag
    value_id() const
    {
        return {};
    }
    bool
    has_value() const override
    {
        return false;
    }
    // Since this never has a value, none of this should ever be called.
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
    move_out() const override
    {
        throw nullptr;
    }
    Value&
    destructive_ref() const override
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
    // Since this is never ready to write, none of this should ever be called.
    // LCOV_EXCL_START
    void
    write(Value) const override
    {
    }
    void
    clear() const override
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

// `default_initialized<Value>()` creates a read-only signal whose value is a
// default-initialized value of type `Value`.
template<class Value>
struct default_initialized_view
    : signal<
          default_initialized_view<Value>,
          Value,
          view_caps<signal_move_activated>,
          constant_value_tag>
{
    default_initialized_view()
    {
    }
    bool
    has_value() const override
    {
        return true;
    }
    constant_value_tag
    value_id() const
    {
        return {};
    }
    Value const&
    read() const override
    {
        return value_;
    }
    Value
    move_out() const override
    {
        return Value();
    }
    Value&
    destructive_ref() const override
    {
        return value_;
    }

 private:
    mutable Value value_{};
};
template<class Value>
default_initialized_view<Value>
default_initialized()
{
    return default_initialized_view<Value>();
}

// `value(v)` creates a read-only signal that carries the value `v`.
template<class Value>
struct value_view
    : regular_signal<
          value_view<Value>,
          Value,
          view_caps<signal_move_activated>>
{
    explicit value_view(Value v) : v_(std::move(v))
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
    move_out() const override
    {
        Value moved = std::move(v_);
        return moved;
    }
    Value&
    destructive_ref() const override
    {
        return v_;
    }

 private:
    mutable Value v_;
};
template<class Value>
value_view<Value>
value(Value v)
{
    return value_view<Value>(std::move(v));
}

// This is a special overload of `value()` for C-style string literals - The
// identity of the signal is simply the address of the literal.
struct string_literal_view
    : lazy_signal<
          string_literal_view,
          std::string,
          view_caps<signal_move_activated>,
          char const*>
{
    string_literal_view(char const* x) : text_(x)
    {
    }
    char const*
    value_id() const
    {
        return text_;
    }
    bool
    has_value() const override
    {
        return true;
    }
    std::string
    move_out() const override
    {
        return std::string(text_);
    }

 private:
    char const* text_;
};
inline string_literal_view
value(char const* text)
{
    return string_literal_view(text);
}

// literal operators
namespace literals {
inline string_literal_view
operator""_a(char const* s, size_t)
{
    return string_literal_view(s);
}
} // namespace literals

// `ref(x)`, where `x` is a non-const reference, creates a binding that
// directly exposes the value of x.
template<class Value>
struct pointer_binding
    : regular_signal<
          pointer_binding<Value>,
          Value,
          binding_caps<signal_movable>>
{
    explicit pointer_binding(Value* v) : v_(v)
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
    move_out() const override
    {
        Value moved = std::move(*v_);
        return moved;
    }
    Value&
    destructive_ref() const override
    {
        return *v_;
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
pointer_binding<Value>
ref(Value& x)
{
    return pointer_binding<Value>(&x);
}

// `ref(x)`, where `x` is a const reference, creates a view that directly
// exposes the value of `x`.
template<class Value>
struct pointer_view
    : regular_signal<pointer_view<Value>, Value, view_caps<signal_readable>>
{
    explicit pointer_view(Value const* v) : v_(v)
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
pointer_view<Value>
ref(Value const& x)
{
    return pointer_view<Value>(&x);
}

// `versioned_ref(x, version)` creates a binding to `x` that uses `version` as
// its value ID. Use this for objects that are not `identifiable` (e.g.
// containers) when you already maintain a revision counter.
//
// If `x` and `version` are mutable, the result is a binding, and writes
// (including destructive movement) increment `version`. If both are const,
// the result is a view.

template<class Value, std::unsigned_integral Version>
struct versioned_pointer_binding
    : signal<
          versioned_pointer_binding<Value, Version>,
          Value,
          binding_caps<signal_movable>,
          Version>
{
    versioned_pointer_binding(Value* v, Version* version)
        : v_(v), version_(version)
    {
    }
    Version const&
    value_id() const
    {
        return *version_;
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
    move_out() const override
    {
        ++*version_;
        Value moved = std::move(*v_);
        return moved;
    }
    Value&
    destructive_ref() const override
    {
        ++*version_;
        return *v_;
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
        ++*version_;
    }

 private:
    Value* v_;
    Version* version_;
};
template<class Value, std::unsigned_integral Version>
versioned_pointer_binding<Value, Version>
versioned_ref(Value& x, Version& version)
{
    return versioned_pointer_binding<Value, Version>(&x, &version);
}

template<class Value, std::unsigned_integral Version>
struct versioned_pointer_view
    : signal<
          versioned_pointer_view<Value, Version>,
          Value,
          view_caps<signal_readable>,
          Version>
{
    versioned_pointer_view(Value const* v, Version const* version)
        : v_(v), version_(version)
    {
    }
    Version const&
    value_id() const
    {
        return *version_;
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
    Version const* version_;
};
template<class Value, std::unsigned_integral Version>
versioned_pointer_view<Value, Version>
versioned_ref(Value const& x, Version const& version)
{
    return versioned_pointer_view<Value, Version>(&x, &version);
}

// `signalize(x)` turns `x` into a signal if it isn't already one.
template<signal_type Signal>
Signal
signalize(Signal s)
{
    return std::move(s);
}
template<class Value>
    requires(!signal_type<Value>)
auto
signalize(Value v)
{
    return value(std::move(v));
}

} // namespace alia
