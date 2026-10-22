#pragma once

#include <alia/impl/context.hpp>

#include <alia/kernel/effects.hpp>
#include <alia/kernel/signals/core.hpp>
#include <alia/kernel/signals/utilities.hpp>

#include <optional>
#include <string>
#include <type_traits>
#include <utility>

// This file defines utilities for constructing basic signals.

namespace alia {

// `empty<Value>()` gives a signal that never has a value.
template<class Value>
struct empty_signal
    : stored_signal<
          empty_signal<Value>,
          Value,
          binding_caps<signal_readable, signal_clearable>,
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
    std::optional<constant_value_tag>
    post_write(kernel_context, Value) const override
    {
        return std::nullopt;
    }
    std::optional<constant_value_tag>
    post_clear(kernel_context) const override
    {
        return std::nullopt;
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
    : stored_signal<
          default_initialized_view<Value>,
          Value,
          view_caps<signal_readable, signal_nonempty>,
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
    : regular_stored_signal<
          value_view<Value>,
          Value,
          view_caps<signal_readable, signal_nonempty>>
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

 private:
    Value v_;
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
          view_caps<signal_readable, signal_nonempty>,
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
    void
    read_into(std::string* dst) const override
    {
        *dst = std::string(text_);
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
    : regular_stored_signal<
          pointer_binding<Value>,
          Value,
          binding_caps<signal_movable, signal_writable, signal_nonempty>>
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
    Value&
    durable_ref() const override
    {
        return *v_;
    }
    bool
    ready_to_write() const override
    {
        return true;
    }
    std::optional<Value>
    post_write(kernel_context ctx, Value value) const override
    {
        Value id = value;
        post_assignment(ctx, v_, std::move(value));
        return id;
    }
    std::optional<Value>
    post_mutation_commit(kernel_context) const override
    {
        // Value-as-ID signals cannot predict the ID of an arbitrary in-place
        // mutation.
        return std::nullopt;
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
    : regular_stored_signal<
          pointer_view<Value>,
          Value,
          view_caps<signal_readable, signal_nonempty>>
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

// `ref_with_revision(x, revision)` creates a binding to `x` that uses
// `revision` as its value ID. Use this for objects that are not
// `identifiable` (e.g. containers) when you already maintain a revision
// counter.
//
// If `x` and `revision` are mutable, the result is a binding, and writes
// (including destructive movement) increment `revision`. If both are const,
// the result is a view.

template<class Value, std::unsigned_integral Revision>
struct pointer_binding_with_revision
    : stored_signal<
          pointer_binding_with_revision<Value, Revision>,
          Value,
          binding_caps<signal_movable, signal_writable, signal_nonempty>,
          Revision>
{
    pointer_binding_with_revision(Value* v, Revision* revision)
        : v_(v), revision_(revision)
    {
    }
    Revision const&
    value_id() const
    {
        return *revision_;
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
    Value&
    durable_ref() const override
    {
        return *v_;
    }
    bool
    ready_to_write() const override
    {
        return true;
    }
    std::optional<Revision>
    post_write(kernel_context ctx, Value value) const override
    {
        Revision const expected = *revision_ + 1;
        post_call(
            ctx,
            [v = v_,
             revision = revision_,
             value = std::move(value)]() mutable {
                *v = std::move(value);
                ++*revision;
            });
        return expected;
    }
    std::optional<Revision>
    post_mutation_commit(kernel_context ctx) const override
    {
        Revision const expected = *revision_ + 1;
        post_call(ctx, [revision = revision_]() { ++*revision; });
        return expected;
    }

 private:
    Value* v_;
    Revision* revision_;
};
template<class Value, std::unsigned_integral Revision>
pointer_binding_with_revision<Value, Revision>
ref_with_revision(Value& x, Revision& revision)
{
    return pointer_binding_with_revision<Value, Revision>(&x, &revision);
}

template<class Value, std::unsigned_integral Revision>
struct pointer_view_with_revision
    : stored_signal<
          pointer_view_with_revision<Value, Revision>,
          Value,
          view_caps<signal_readable, signal_nonempty>,
          Revision>
{
    pointer_view_with_revision(Value const* v, Revision const* revision)
        : v_(v), revision_(revision)
    {
    }
    Revision const&
    value_id() const
    {
        return *revision_;
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
    Revision const* revision_;
};
template<class Value, std::unsigned_integral Revision>
pointer_view_with_revision<Value, Revision>
ref_with_revision(Value const& x, Revision const& revision)
{
    return pointer_view_with_revision<Value, Revision>(&x, &revision);
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
