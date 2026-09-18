#pragma once

#include <alia/abi/context.h>
#include <alia/kernel/id.hpp>

#include <cassert>
#include <concepts>
#include <exception>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <alia/abi/base/arena.h>
#include <alia/abi/prelude.h>

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
// The signal can return a const reference to its value and can copy the value
// into a destination provided by the caller.
constexpr unsigned signal_readable = 0b0001;
// The signal exposes a `durable_ref()` to resident storage that remains valid
// outside the current pass. Writable durable signals also support
// `post_mutation_commit()`.
constexpr unsigned signal_durable = 0b0011;
// The signal may be opened for move-out via `alia::move`.
constexpr unsigned signal_movable = 0b0111;
// Movement is activated on this signal. Its value may be stolen via an effect.
constexpr unsigned signal_move_activated = 0b1111;

// The following are the same, but for writing.
constexpr unsigned signal_unwritable = 0b00;
constexpr unsigned signal_writable = 0b01;
constexpr unsigned signal_clearable = 0b11;

// Presence is a read-side guarantee about `has_value()`.
// `signal_maybe_empty` is the default: `has_value()` may be false.
// `signal_nonempty` means `has_value()` is always true.
constexpr unsigned signal_maybe_empty = 0b0;
constexpr unsigned signal_nonempty = 0b1;

// combined capabilities
template<
    unsigned Reading,
    unsigned Writing,
    unsigned Presence = signal_maybe_empty>
struct signal_capabilities
{
    static constexpr unsigned reading = Reading;
    static constexpr unsigned writing = Writing;
    static constexpr unsigned presence = Presence;
};

// Signals in Alia are grouped into three basic roles:
// - view: a read-only signal
// - binding: a bidirectional signal
// - sink: a write-only signal (much less common than the other two)

// The following allow you to construct capability packs for the various roles
// at varying levels of capability.

template<unsigned Reading, unsigned Presence = signal_maybe_empty>
using view_caps = signal_capabilities<Reading, signal_unwritable, Presence>;

template<unsigned Writing, unsigned Presence = signal_maybe_empty>
using sink_caps = signal_capabilities<signal_unreadable, Writing, Presence>;

template<
    unsigned Reading,
    unsigned Writing = signal_writable,
    unsigned Presence = signal_maybe_empty>
using binding_caps = signal_capabilities<Reading, Writing, Presence>;

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
   && signal_capability_level_is_compatible<Expected::writing, Actual::writing>
   && signal_capability_level_is_compatible<
          Expected::presence,
          Actual::presence>;

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
    signal_capability_level_intersection<A::writing, B::writing>,
    signal_capability_level_intersection<A::presence, B::presence>>;

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
    signal_capability_level_union<A::writing, B::writing>,
    signal_capability_level_union<A::presence, B::presence>>;

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
    // Concrete signals expose `value_id()`, which returns a concrete value ID.
    // `value_id_erased()` (below) returns a type-erased `id_view` of the same
    // ID.
    //
    // The ID is required to be valid if (and only if) the signal has a value.
    // If there is no value, `value_id()` is effectively undefined and
    // `value_id_erased()` returns null_id() .
    //
    // The returned ID is a transient view and is only guaranteed to be valid
    // as long as the signal itself is valid.
    //
    virtual id_view
    value_id_erased() const = 0;

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

    // Read the signal's value into `*dst`. Signals that lazily generate their
    // value may construct directly into `*dst`. Others typically assign from
    // their stored value.
    virtual void
    read_into(Value* dst) const = 0;

    // Get a reference to the signal's resident value. The reference remains
    // valid outside the current pass.
    //
    // If the signal is also writable, this reference may be used to post
    // mutations to the signal value. In that case, `post_mutation_commit` must
    // be called after the mutation is posted.
    //
    // This reference can also be used for move-outs if the signal is
    // move-activated.
    //
    virtual Value&
    durable_ref() const = 0;

    // Post a write of `value` into the signal and return a type-erased version
    // of the value ID that will be presented after the write runs. The ID view
    // returned here is only valid until the pass scratch is reset, and it can
    // be `null_id()` if the new value ID is unknown.
    virtual id_view
    post_write_erased(alia_context* ctx, Value value) const = 0;

    // Post a clear of the signal and return a type-erased version of the value
    // ID that will be presented after the clear runs. The ID view returned
    // here is only valid until the pass scratch is reset, and it can be
    // `null_id()` if the new value ID is unknown.
    virtual id_view
    post_clear_erased(alia_context* ctx) const = 0;

    // Post bookkeeping for an in-place mutation (or steal) performed through
    // `durable_ref()`. Return a type-erased predicted value ID. The ID view
    // is only valid until the pass scratch is reset, and it can be `null_id()`
    // if the new value ID is unknown.
    virtual id_view
    post_mutation_commit_erased(alia_context* ctx) const = 0;
};

namespace detail {

// Erase a typed value ID into an `id_view`. For ID types that require external
// storage (e.g. ID pairs), the storage is allocated from the pass scratch so
// the returned view remains valid for the rest of the pass.
template<class ValueId>
id_view
erase_value_id(alia_context* ctx, ValueId const& id)
{
    using storage_type = erased_id_storage<ValueId>;
    if constexpr (std::is_empty_v<storage_type>)
    {
        storage_type storage{};
        return to_id_view(storage, id);
    }
    else
    {
        ALIA_ASSERT(ctx);
        ALIA_ASSERT(ctx->scratch);
        size_t const bytes = alia_min_aligned_size(sizeof(storage_type));
        size_t const align = alignof(storage_type) < ALIA_MIN_ALIGN
                               ? ALIA_MIN_ALIGN
                               : alignof(storage_type);
        void* mem = alia_arena_ptr(
            ctx->scratch,
            alia_arena_alloc_aligned(ctx->scratch, bytes, align));
        auto* storage = new (mem) storage_type{};
        return to_id_view(*storage, id);
    }
}

} // namespace detail

template<class Derived, class Value, class Capabilities, class ValueId>
struct signal_base : signal_interface<Value>
{
    using capabilities = Capabilities;
    using value_id_type = ValueId;

    // Yield a signal to the element at `index`. `index` may be a signal or a
    // raw value. The definition is in `operators.hpp`.
    template<class Index>
    auto
    operator[](Index index) const;

    // Erase this signal's typed value ID into an `id_view`. Concrete signals
    // define a typed `value_id()` that returns `value_id_type`.
    id_view
    value_id_erased() const override
    {
        Derived const& self = static_cast<Derived const&>(*this);
        if (!self.has_value())
            return null_id();
        return to_id_view(value_id_storage_, self.value_id());
    }

    // Post a write of `value` and return the value ID that will be presented
    // after the write runs. (If that ID is unknown, `std::nullopt` can be
    // returned.)
    virtual std::optional<ValueId>
    post_write(alia_context* ctx, Value value) const = 0;

    // Post a clear and return the value ID that will be presented after the
    // clear runs. (If that ID is unknown, `std::nullopt` can be returned.)
    virtual std::optional<ValueId>
    post_clear(alia_context* ctx) const = 0;

    // Post bookkeeping for an in-place mutation (or steal) performed through
    // `durable_ref()`. Return the value ID that will be presented afterward.
    // (If that ID is unknown, `std::nullopt` can be returned.)
    // Requires the signal to currently have a value and be ready to write.
    virtual std::optional<ValueId>
    post_mutation_commit(alia_context* ctx) const = 0;

    id_view
    post_write_erased(alia_context* ctx, Value value) const override
    {
        auto id = post_write(ctx, std::move(value));
        if (!id)
            return null_id();
        return detail::erase_value_id(ctx, *id);
    }

    id_view
    post_clear_erased(alia_context* ctx) const override
    {
        auto id = post_clear(ctx);
        if (!id)
            return null_id();
        return detail::erase_value_id(ctx, *id);
    }

    id_view
    post_mutation_commit_erased(alia_context* ctx) const override
    {
        auto id = post_mutation_commit(ctx);
        if (!id)
            return null_id();
        return detail::erase_value_id(ctx, *id);
    }

 protected:
    // persistent storage for erasing `value_id_type`
    mutable erased_id_storage<ValueId> value_id_storage_{};
};

template<class Derived, class Value, class Capabilities, class ValueId>
struct signal : signal_base<Derived, Value, Capabilities, ValueId>
{
};

// The following implement the various unused functions that are required by
// `signal_interface` but won't be used because of the capabilities of the
// signal. Presence is a free parameter so nonempty packs share these stubs.

// LCOV_EXCL_START

#define ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()                           \
    std::optional<ValueId> post_clear(alia_context*) const override           \
    {                                                                         \
        return std::nullopt;                                                  \
    }

#define ALIA_DEFINE_UNUSED_SIGNAL_MUTATION_COMMIT_INTERFACE()                 \
    std::optional<ValueId> post_mutation_commit(alia_context*) const override \
    {                                                                         \
        return std::nullopt;                                                  \
    }

#define ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)                      \
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()                               \
    ALIA_DEFINE_UNUSED_SIGNAL_MUTATION_COMMIT_INTERFACE()                     \
    bool ready_to_write() const override                                      \
    {                                                                         \
        return false;                                                         \
    }                                                                         \
    std::optional<ValueId> post_write(alia_context*, Value) const override    \
    {                                                                         \
        return std::nullopt;                                                  \
    }

#define ALIA_DEFINE_UNUSED_SIGNAL_DURABLE_REF_INTERFACE(Value)                \
    Value& durable_ref() const override                                       \
    {                                                                         \
        throw nullptr;                                                        \
    }

#define ALIA_DEFINE_UNUSED_SIGNAL_READ_INTERFACE(Value)                       \
    ALIA_DEFINE_UNUSED_SIGNAL_DURABLE_REF_INTERFACE(Value)                    \
    ALIA_DEFINE_UNUSED_SIGNAL_MUTATION_COMMIT_INTERFACE()                     \
    ValueId value_id() const                                                  \
    {                                                                         \
        return {};                                                            \
    }                                                                         \
    bool has_value() const override                                           \
    {                                                                         \
        return false;                                                         \
    }                                                                         \
    Value const& read() const override                                        \
    {                                                                         \
        throw nullptr;                                                        \
    }                                                                         \
    void read_into(Value*) const override                                     \
    {                                                                         \
        throw nullptr;                                                        \
    }

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<Derived, Value, view_caps<signal_readable, Presence>, ValueId>
    : signal_base<
          Derived,
          Value,
          view_caps<signal_readable, Presence>,
          ValueId>
{
    ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)
    ALIA_DEFINE_UNUSED_SIGNAL_DURABLE_REF_INTERFACE(Value)
};

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<Derived, Value, view_caps<signal_durable, Presence>, ValueId>
    : signal_base<Derived, Value, view_caps<signal_durable, Presence>, ValueId>
{
    ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)
};

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<Derived, Value, view_caps<signal_movable, Presence>, ValueId>
    : signal_base<Derived, Value, view_caps<signal_movable, Presence>, ValueId>
{
    ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)
};

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<
    Derived,
    Value,
    view_caps<signal_move_activated, Presence>,
    ValueId>
    : signal_base<
          Derived,
          Value,
          view_caps<signal_move_activated, Presence>,
          ValueId>
{
    ALIA_DEFINE_UNUSED_SIGNAL_WRITE_INTERFACE(Value)
};

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<Derived, Value, sink_caps<signal_writable, Presence>, ValueId>
    : signal_base<
          Derived,
          Value,
          sink_caps<signal_writable, Presence>,
          ValueId>
{
    ALIA_DEFINE_UNUSED_SIGNAL_READ_INTERFACE(Value)
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<
    Derived,
    Value,
    binding_caps<signal_readable, signal_writable, Presence>,
    ValueId>
    : signal_base<
          Derived,
          Value,
          binding_caps<signal_readable, signal_writable, Presence>,
          ValueId>
{
    ALIA_DEFINE_UNUSED_SIGNAL_DURABLE_REF_INTERFACE(Value)
    ALIA_DEFINE_UNUSED_SIGNAL_MUTATION_COMMIT_INTERFACE()
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<
    Derived,
    Value,
    binding_caps<signal_durable, signal_writable, Presence>,
    ValueId>
    : signal_base<
          Derived,
          Value,
          binding_caps<signal_durable, signal_writable, Presence>,
          ValueId>
{
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<
    Derived,
    Value,
    binding_caps<signal_movable, signal_writable, Presence>,
    ValueId>
    : signal_base<
          Derived,
          Value,
          binding_caps<signal_movable, signal_writable, Presence>,
          ValueId>
{
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<
    Derived,
    Value,
    binding_caps<signal_move_activated, signal_writable, Presence>,
    ValueId>
    : signal_base<
          Derived,
          Value,
          binding_caps<signal_move_activated, signal_writable, Presence>,
          ValueId>
{
    ALIA_DEFINE_UNUSED_SIGNAL_CLEAR_INTERFACE()
};

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<
    Derived,
    Value,
    binding_caps<signal_readable, signal_clearable, Presence>,
    ValueId>
    : signal_base<
          Derived,
          Value,
          binding_caps<signal_readable, signal_clearable, Presence>,
          ValueId>
{
    ALIA_DEFINE_UNUSED_SIGNAL_DURABLE_REF_INTERFACE(Value)
    ALIA_DEFINE_UNUSED_SIGNAL_MUTATION_COMMIT_INTERFACE()
};

template<class Derived, class Value, unsigned Presence, class ValueId>
struct signal<
    Derived,
    Value,
    binding_caps<signal_durable, signal_clearable, Presence>,
    ValueId>
    : signal_base<
          Derived,
          Value,
          binding_caps<signal_durable, signal_clearable, Presence>,
          ValueId>
{
};

// LCOV_EXCL_STOP

// signal_ref is a reference to a signal that acts as a signal itself.
template<class Value, class Capabilities>
struct signal_ref
    : signal<signal_ref<Value, Capabilities>, Value, Capabilities, id_view>
{
    template<class V, class C>
    friend struct signal_ref;

    // Construct from any signal with compatible capabilities.
    template<class OtherSignal, class OtherCapabilities, class OtherValueId>
        requires signal_capabilities_compatible<
            Capabilities,
            OtherCapabilities>
    signal_ref(
        signal<OtherSignal, Value, OtherCapabilities, OtherValueId> const& s)
        : ref_(&s)
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
    void
    read_into(Value* dst) const override
    {
        ref_->read_into(dst);
    }
    Value&
    durable_ref() const override
    {
        return ref_->durable_ref();
    }
    id_view
    value_id() const
    {
        return ref_->value_id_erased();
    }
    bool
    ready_to_write() const override
    {
        return ref_->ready_to_write();
    }
    id_view
    post_write_erased(alia_context* ctx, Value value) const override
    {
        return ref_->post_write_erased(ctx, std::move(value));
    }
    id_view
    post_clear_erased(alia_context* ctx) const override
    {
        return ref_->post_clear_erased(ctx);
    }
    id_view
    post_mutation_commit_erased(alia_context* ctx) const override
    {
        return ref_->post_mutation_commit_erased(ctx);
    }
    std::optional<id_view>
    post_write(alia_context* ctx, Value value) const override
    {
        id_view const id = post_write_erased(ctx, std::move(value));
        if (alia_id_view_is_null(id))
            return std::nullopt;
        return id;
    }
    std::optional<id_view>
    post_clear(alia_context* ctx) const override
    {
        id_view const id = post_clear_erased(ctx);
        if (alia_id_view_is_null(id))
            return std::nullopt;
        return id;
    }
    std::optional<id_view>
    post_mutation_commit(alia_context* ctx) const override
    {
        id_view const id = post_mutation_commit_erased(ctx);
        if (alia_id_view_is_null(id))
            return std::nullopt;
        return id;
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
concept signal_type = requires {
    sizeof(T);
    requires std::is_base_of_v<untyped_signal_base, T>;
};

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
ALIA_DEFINE_SIGNAL_SUGAR(binding, binding_caps<signal_readable>)
using nonempty_view_caps = view_caps<signal_readable, signal_nonempty>;
using nonempty_binding_caps
    = binding_caps<signal_readable, signal_writable, signal_nonempty>;
ALIA_DEFINE_SIGNAL_SUGAR(nonempty_view, nonempty_view_caps)
ALIA_DEFINE_SIGNAL_SUGAR(nonempty_binding, nonempty_binding_caps)

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
// Unlike calling signal.read() directly, this will generate a
// compile-time error if the signal's type doesn't support reading.
// If the signal is not known to be nonempty, a run-time assert checks that it
// currently has a value.
template<view_signal Signal>
Signal::value_type const&
read_signal(Signal const& signal)
{
    if constexpr (!nonempty_view_signal<Signal>)
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

// Post a write of `value` into `signal`.
// Unlike calling signal.post_write() directly, this will generate a
// compile-time error if the signal's type doesn't support writing.
// If the signal isn't ready to write, this is a no-op and returns
// `std::nullopt`.
template<sink_signal Signal, class Value>
std::optional<typename Signal::value_id_type>
write_signal(alia_context* ctx, Signal const& signal, Value value)
{
    if (signal.ready_to_write())
    {
        try
        {
            return signal.post_write(ctx, std::move(value));
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
    return std::nullopt;
}

// `by_value_signal_capture` stores a captured signal value as a copy.
template<class T>
struct by_value_signal_capture
{
    explicit by_value_signal_capture(T value) : value_(std::move(value))
    {
    }

    T
    take()
    {
        return std::move(value_);
    }

 private:
    T value_;
};

// `by_ref_signal_capture` stores a signal value that has been captured via
// a durable reference.
template<class T>
struct by_ref_signal_capture
{
    explicit by_ref_signal_capture(T* ptr) : ptr_(ptr)
    {
        ALIA_ASSERT(ptr_);
    }

    T
    take()
    {
        return std::move(*ptr_);
    }

 private:
    T* ptr_;
};

// capture_signal_value() captures a signal's value for use in an effect.
// It chooses the most efficient mechanism for storing the value so that it
// can be retained until the effect is run and then forwarded elsewhere.
//
// If the capture is used to post mutating effects, you must then call
// `commit_signal_capture_mutation()` to post any required mutation
// bookkeeping.

template<view_signal Signal>
    requires signal_with<Signal, view_caps<signal_move_activated>>
by_ref_signal_capture<typename Signal::value_type>
capture_signal_value(Signal const& signal)
{
    return by_ref_signal_capture<typename Signal::value_type>(
        &signal.durable_ref());
}

template<view_signal Signal>
    requires(!signal_with<Signal, view_caps<signal_move_activated>>)
by_value_signal_capture<typename Signal::value_type>
capture_signal_value(Signal const& signal)
{
    typename Signal::value_type value{};
    signal.read_into(&value);
    return by_value_signal_capture<typename Signal::value_type>(
        std::move(value));
}

// Post bookkeeping for an in-place mutation of `signal` through a captured
// reference.
template<class T, class Signal>
void
commit_signal_capture_mutation(
    alia_context* ctx,
    by_ref_signal_capture<T> const& captured,
    Signal const& signal)
{
    (void) captured;
    signal.post_mutation_commit(ctx);
}

// `commit_signal_capture_mutation()` is a no-op for a by-value capture.
// The source signal is not mutated.
template<class T, class Signal>
void
commit_signal_capture_mutation(
    alia_context* ctx,
    by_value_signal_capture<T> const& captured,
    Signal const& signal)
{
    (void) ctx;
    (void) captured;
    (void) signal;
}

// Post bookkeeping for an in-place mutation of `signal` through
// `durable_ref()`. If the signal has no value or isn't ready to write, this
// is a no-op and returns `std::nullopt`.
template<signal_with<binding_caps<signal_durable, signal_writable>> Signal>
std::optional<typename Signal::value_id_type>
commit_signal_mutation(alia_context* ctx, Signal const& signal)
{
    if (signal.has_value() && signal.ready_to_write())
        return signal.post_mutation_commit(ctx);
    return std::nullopt;
}

// Post a clear of `signal`.
// Unlike calling signal.post_clear() directly, this will generate a
// compile-time error if the signal's type doesn't support clearing.
// If the signal isn't ready to write, this is a no-op and returns
// `std::nullopt`.
template<signal_with<sink_caps<signal_clearable>> Signal>
std::optional<typename Signal::value_id_type>
clear_signal(alia_context* ctx, Signal const& signal)
{
    if (signal.ready_to_write())
        return signal.post_clear(ctx);
    return std::nullopt;
}

} // namespace alia
