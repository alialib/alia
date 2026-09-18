#pragma once

#include <alia/kernel/actions/core.hpp>
#include <alia/kernel/signals/adaptors.hpp>
#include <alia/kernel/signals/application.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/core.hpp>
#include <alia/kernel/signals/utilities.hpp>

#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>

// This file defines operators for signals.

namespace alia {

// Given a signal to a structure, `structure->*field` yields a signal to that
// field. (`field` is a pointer-to-member.)
//
// The field signal's capabilities are the intersection of the structure's
// capabilities with a durable binding. The field is durable when the parent
// is, but not movable. (Move-out of a single field is not supported.)
//
// When the parent is a writable durable signal, writes assign through
// `durable_ref` and call `post_mutation_commit`. Otherwise writes copy the
// structure, assign the field, and write the structure back.
//
// If the field type is `identifiable`, the value ID is the field value.
// Otherwise it is the structure ID paired with an ID for the field pointer.
//
template<class StructureSignal, class Field>
struct field_signal
    : stored_signal<
          field_signal<StructureSignal, Field>,
          Field,
          signal_capabilities_intersection<
              typename StructureSignal::capabilities,
              binding_caps<signal_durable, signal_writable, signal_nonempty>>,
          std::conditional_t<
              identifiable<Field>,
              Field,
              std::pair<
                  typename StructureSignal::value_id_type,
                  Field StructureSignal::value_type::*>>>
{
    using structure_type = typename StructureSignal::value_type;
    using field_ptr = Field structure_type::*;
    using value_id_type = std::conditional_t<
        identifiable<Field>,
        Field,
        std::pair<
            typename StructureSignal::value_id_type,
            Field StructureSignal::value_type::*>>;

    field_signal(StructureSignal structure, field_ptr field)
        : structure_(std::move(structure)), field_(field)
    {
    }
    bool
    has_value() const override
    {
        return structure_.has_value();
    }
    Field const&
    read() const override
    {
        return structure_.read().*field_;
    }
    Field&
    durable_ref() const override
    {
        return structure_.durable_ref().*field_;
    }
    decltype(auto)
    value_id() const
    {
        if constexpr (identifiable<Field>)
            return this->read();
        else
            return std::pair{structure_.value_id(), field_};
    }
    bool
    ready_to_write() const override
    {
        return structure_.has_value() && structure_.ready_to_write();
    }
    std::optional<value_id_type>
    post_write(alia_context* ctx, Field x) const override
    {
        if constexpr (
            signal_with<
                StructureSignal,
                binding_caps<signal_durable, signal_writable>>)
        {
            Field* field_ptr_addr = &(structure_.durable_ref().*field_);
            if constexpr (identifiable<Field>)
            {
                Field id = x;
                post_assignment(ctx, field_ptr_addr, std::move(x));
                (void) structure_.post_mutation_commit(ctx);
                return id;
            }
            else
            {
                post_assignment(ctx, field_ptr_addr, std::move(x));
                auto structure_id = structure_.post_mutation_commit(ctx);
                if (!structure_id)
                    return std::nullopt;
                return value_id_type{std::move(*structure_id), field_};
            }
        }
        else
        {
            if constexpr (identifiable<Field>)
            {
                Field id = x;
                structure_type s(read_signal(structure_));
                s.*field_ = std::move(x);
                structure_.post_write(ctx, std::move(s));
                return id;
            }
            else
            {
                structure_type s(read_signal(structure_));
                s.*field_ = std::move(x);
                auto structure_id = structure_.post_write(ctx, std::move(s));
                if (!structure_id)
                    return std::nullopt;
                return value_id_type{std::move(*structure_id), field_};
            }
        }
    }
    std::optional<value_id_type>
    post_mutation_commit(alia_context* ctx) const override
    {
        if constexpr (
            signal_with<
                StructureSignal,
                binding_caps<signal_durable, signal_writable>>)
        {
            auto structure_id = structure_.post_mutation_commit(ctx);
            if constexpr (identifiable<Field>)
            {
                (void) structure_id;
                return std::nullopt;
            }
            else
            {
                if (!structure_id)
                    return std::nullopt;
                return value_id_type{std::move(*structure_id), field_};
            }
        }
        else
        {
            return std::nullopt;
        }
    }

 private:
    StructureSignal structure_;
    field_ptr field_;
};

template<view_signal StructureSignal, class Field>
field_signal<StructureSignal, Field>
operator->*(
    StructureSignal const& structure,
    Field StructureSignal::value_type::* field)
{
    return field_signal<StructureSignal, Field>(structure, field);
}

// `ALIA_FIELD(x, f)` is equivalent to `x->*T::f` where `T` is the value type
// of `x`.
#define ALIA_FIELD(x, f) ((x)->*&std::decay<decltype(read_signal(x))>::type::f)
#ifndef ALIA_STRICT_MACROS
#define alia_field(x, f) ALIA_FIELD(x, f)
#endif

#define ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(op)                                \
    template<view_signal A, view_signal B>                                    \
    auto operator op(A const& a, B const& b)                                  \
    {                                                                         \
        return uncached_apply([](auto a, auto b) { return a op b; }, a, b);   \
    }

ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(+)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(-)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(*)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(/)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(^)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(%)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(&)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(|)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(<<)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(>>)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(==)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(!=)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(<)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(<=)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(>)
ALIA_DEFINE_BINARY_SIGNAL_OPERATOR(>=)

#undef ALIA_DEFINE_BINARY_SIGNAL_OPERATOR

#define ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(op)                        \
    template<view_signal A, class B>                                          \
        requires(!signal_type<B>)                                             \
    auto operator op(A const& a, B const& b)                                  \
    {                                                                         \
        return uncached_apply(                                                \
            [](auto a, auto b) { return a op b; }, a, value(b));              \
    }                                                                         \
    template<class A, view_signal B>                                          \
        requires(!signal_type<A> && !action_type<A>)                          \
    auto operator op(A const& a, B const& b)                                  \
    {                                                                         \
        return uncached_apply(                                                \
            [](auto a, auto b) { return a op b; }, value(a), b);              \
    }

ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(+)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(-)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(*)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(/)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(^)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(%)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(&)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(|)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(<<)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(>>)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(==)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(!=)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(<)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(<=)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(>)
ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR(>=)

#undef ALIA_DEFINE_LIBERAL_BINARY_SIGNAL_OPERATOR

#define ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(op)                                 \
    template<view_signal A>                                                   \
    auto operator op(A const& a)                                              \
    {                                                                         \
        return uncached_apply([](auto a) { return op a; }, a);                \
    }

ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(-)
ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(!)
ALIA_DEFINE_UNARY_SIGNAL_OPERATOR(*)

#undef ALIA_DEFINE_UNARY_SIGNAL_OPERATOR

// The || and && operators require special implementations because they follow
// the normal short-circuiting evaluation rules of C++.

template<class Arg0, class Arg1>
struct logical_or_signal
    : regular_stored_signal<
          logical_or_signal<Arg0, Arg1>,
          bool,
          view_caps<signal_readable>>
{
    logical_or_signal(Arg0 arg0, Arg1 arg1)
        : arg0_(std::move(arg0)), arg1_(std::move(arg1))
    {
    }
    bool
    has_value() const override
    {
        // This has a value if both arguments have values, or if either
        // argument is true.
        return (arg0_.has_value() && arg1_.has_value())
            || (arg0_.has_value() && arg0_.read())
            || (arg1_.has_value() && arg1_.read());
    }
    bool const&
    read() const override
    {
        value_ = (arg0_.has_value() && arg0_.read())
              || (arg1_.has_value() && arg1_.read());
        return value_;
    }

 private:
    Arg0 arg0_;
    Arg1 arg1_;
    mutable bool value_;
};

template<view_signal A, view_signal B>
auto
operator||(A const& a, B const& b)
{
    return logical_or_signal<A, B>(a, b);
}

template<view_signal A, class B>
    requires(!signal_type<B>)
auto
operator||(A const& a, B const& b)
{
    return a || value(b);
}

template<class A, view_signal B>
    requires(!signal_type<A> && !action_type<A>)
auto
operator||(A const& a, B const& b)
{
    return value(a) || b;
}

template<class Arg0, class Arg1>
struct logical_and_signal
    : regular_stored_signal<
          logical_and_signal<Arg0, Arg1>,
          bool,
          view_caps<signal_readable>>
{
    logical_and_signal(Arg0 arg0, Arg1 arg1)
        : arg0_(std::move(arg0)), arg1_(std::move(arg1))
    {
    }
    bool
    has_value() const override
    {
        // This has a value if both arguments have values, or if either
        // argument is false.
        return (arg0_.has_value() && arg1_.has_value())
            || (arg0_.has_value() && !arg0_.read())
            || (arg1_.has_value() && !arg1_.read());
    }
    bool const&
    read() const override
    {
        value_
            = !((arg0_.has_value() && !arg0_.read())
                || (arg1_.has_value() && !arg1_.read()));
        return value_;
    }

 private:
    Arg0 arg0_;
    Arg1 arg1_;
    mutable bool value_;
};

template<view_signal A, view_signal B>
auto
operator&&(A const& a, B const& b)
{
    return logical_and_signal<A, B>(a, b);
}

template<view_signal A, class B>
    requires(!signal_type<B>)
auto
operator&&(A const& a, B const& b)
{
    return a && value(b);
}

template<class A, view_signal B>
    requires(!signal_type<A> && !action_type<A>)
auto
operator&&(A const& a, B const& b)
{
    return value(a) && b;
}

// `conditional(b, t, f)`, where `b`, `t`, and `f` are signals, yields `t` if
// `b`'s value is true and `f` if `b`'s value is false.
//
// `t` and `f` must have the same value type, and `b`'s value type must be
// testable in a boolean context.
//
// This is a normal function call, so unlike an if statement or the ternary
// operator, both `t` and `f` are constructed. (But the a signal's value will
// only be touched if it is selected by the condition.)
//
template<class Condition, class T, class F>
    requires identifiable<typename T::value_type>
struct signal_mux
    : regular_stored_signal<
          signal_mux<Condition, T, F>,
          typename T::value_type,
          signal_capabilities<
              signal_capability_level_intersection<
                  T::capabilities::reading,
                  F::capabilities::reading>,
              signal_capability_level_intersection<
                  T::capabilities::writing,
                  F::capabilities::writing>,
              signal_capability_level_intersection<
                  signal_capability_level_intersection<
                      T::capabilities::presence,
                      F::capabilities::presence>,
                  Condition::capabilities::presence>>>
{
    signal_mux(Condition condition, T t, F f)
        : condition_(std::move(condition)), t_(std::move(t)), f_(std::move(f))
    {
    }
    bool
    has_value() const override
    {
        return condition_.has_value()
            && (condition_.read() ? t_.has_value() : f_.has_value());
    }
    typename T::value_type const&
    read() const override
    {
        return condition_.read() ? t_.read() : f_.read();
    }
    typename T::value_type&
    durable_ref() const override
    {
        return condition_.read() ? t_.durable_ref() : f_.durable_ref();
    }
    bool
    ready_to_write() const override
    {
        return condition_.has_value()
            && (condition_.read() ? t_.ready_to_write() : f_.ready_to_write());
    }
    std::optional<typename T::value_type>
    post_write(alia_context* ctx, typename T::value_type value) const override
    {
        typename T::value_type id = value;
        if (condition_.read())
            t_.post_write(ctx, std::move(value));
        else
            f_.post_write(ctx, std::move(value));
        return id;
    }
    std::optional<typename T::value_type>
    post_clear(alia_context* ctx) const override
    {
        if (condition_.read())
            (void) t_.post_clear(ctx);
        else
            (void) f_.post_clear(ctx);
        return std::nullopt;
    }
    std::optional<typename T::value_type>
    post_mutation_commit(alia_context* ctx) const override
    {
        if (condition_.read())
            (void) t_.post_mutation_commit(ctx);
        else
            (void) f_.post_mutation_commit(ctx);
        return std::nullopt;
    }
    bool
    invalidate(std::exception_ptr error) const override
    {
        if (condition_.read())
            return t_.invalidate(error);
        return f_.invalidate(error);
    }
    bool
    is_invalidated() const override
    {
        if (condition_.read())
            return t_.is_invalidated();
        return f_.is_invalidated();
    }

 private:
    Condition condition_;
    T t_;
    F f_;
};

template<class Condition, class T, class F>
auto
conditional(Condition condition, T t, F f)
{
    auto condition_signal = signalize(std::move(condition));
    auto t_signal = signalize(std::move(t));
    auto f_signal = signalize(std::move(f));
    return signal_mux<
        decltype(condition_signal),
        decltype(t_signal),
        decltype(f_signal)>(
        std::move(condition_signal), std::move(t_signal), std::move(f_signal));
}

// The following support `operator[]` on container signals.

template<class T>
concept has_value_type = requires { typename T::value_type; };

template<class T>
concept has_mapped_type = requires { typename T::mapped_type; };

// `subscript_result_type<Container, Index>::type` is the value type produced
// by subscripting `Container` with `Index`.
//
// The logic is as follows:
// 1 - If the container has a `mapped_type` field, use that.
//     (This covers associative containers.)
// 2 - Otherwise, if the container has a `value_type` field, use that.
//     (This covers sequence containers.)
// 3 - Otherwise, use the decayed return type of `operator[]`.
//
template<class Container, class Index>
struct subscript_result_type
{
    using type = std::decay_t<
        decltype(std::declval<Container>()[std::declval<Index>()])>;
};
template<class Container, class Index>
    requires has_value_type<Container> && (!has_mapped_type<Container>)
struct subscript_result_type<Container, Index>
{
    using type = typename Container::value_type;
};
template<class Container, class Index>
    requires has_mapped_type<Container>
struct subscript_result_type<Container, Index>
{
    using type = typename Container::mapped_type;
};

template<class Container, class Index>
constexpr bool subscript_returns_reference = std::is_reference_v<
    decltype(std::declval<Container>()[std::declval<Index>()])>;

template<class Container, class Index>
concept has_at_indexer = requires(
    Container const& container, Index const& index) { container.at(index); };

template<class Container, class Index>
    requires has_at_indexer<Container, Index>
auto
invoke_const_subscript(Container const& container, Index const& index)
    -> decltype(container.at(index))
{
    return container.at(index);
}

template<class Container, class Index>
    requires(!has_at_indexer<Container, Index>)
auto
invoke_const_subscript(Container const& container, Index const& index)
    -> decltype(container[index])
{
    return container[index];
}

template<class Container, class Index>
constexpr bool const_subscript_returns_reference
    = std::is_reference_v<decltype(invoke_const_subscript(
        std::declval<Container>(), std::declval<Index>()))>;

// Invoke a const subscript and always return a reference. If the container
// yields a proxy or a value, store it so a reference can be returned.
template<class Container, class Index>
struct const_subscript_invoker
{
    auto const&
    operator()(Container const& container, Index const& index) const
    {
        if constexpr (const_subscript_returns_reference<Container, Index>)
        {
            return invoke_const_subscript(container, index);
        }
        else
        {
            storage_ = invoke_const_subscript(container, index);
            return storage_;
        }
    }

 private:
    mutable typename subscript_result_type<Container, Index>::type storage_{};
};

// Given a signal to a container, `container[index]` yields a signal to that
// element. `index` can either be a signal or a raw value.
//
// Reads prefer `at` when the container provides it. Writes move (or copy) the
// container, assign through `operator[]`, and write the container back.
//
// If the element type is `identifiable`, the value ID is the element value.
// Otherwise it is the container ID paired with the index ID.
//
// If `operator[]` returns a reference, the result is durable when the
// container is. Otherwise (proxies such as `std::vector<bool>`), the result
// is not durable and `durable_ref` is unavailable.
//
template<class ContainerSignal, class IndexSignal>
struct subscript_signal
    : stored_signal<
          subscript_signal<ContainerSignal, IndexSignal>,
          typename subscript_result_type<
              typename ContainerSignal::value_type,
              typename IndexSignal::value_type>::type,
          std::conditional_t<
              subscript_returns_reference<
                  typename ContainerSignal::value_type,
                  typename IndexSignal::value_type>,
              signal_capabilities<
                  signal_capability_level_intersection<
                      ContainerSignal::capabilities::reading,
                      signal_durable>,
                  signal_capability_level_intersection<
                      ContainerSignal::capabilities::writing,
                      signal_writable>,
                  signal_capability_level_intersection<
                      ContainerSignal::capabilities::presence,
                      IndexSignal::capabilities::presence>>,
              signal_capabilities<
                  signal_capability_level_intersection<
                      ContainerSignal::capabilities::reading,
                      signal_readable>,
                  signal_capability_level_intersection<
                      ContainerSignal::capabilities::writing,
                      signal_writable>,
                  signal_capability_level_intersection<
                      ContainerSignal::capabilities::presence,
                      IndexSignal::capabilities::presence>>>,
          std::conditional_t<
              identifiable<typename subscript_result_type<
                  typename ContainerSignal::value_type,
                  typename IndexSignal::value_type>::type>,
              typename subscript_result_type<
                  typename ContainerSignal::value_type,
                  typename IndexSignal::value_type>::type,
              std::pair<
                  typename ContainerSignal::value_id_type,
                  typename IndexSignal::value_id_type>>>
{
    using value_type = typename subscript_signal::value_type;
    using value_id_type = std::conditional_t<
        identifiable<value_type>,
        value_type,
        std::pair<
            typename ContainerSignal::value_id_type,
            typename IndexSignal::value_id_type>>;

    subscript_signal(ContainerSignal container, IndexSignal index)
        : container_(std::move(container)), index_(std::move(index))
    {
    }
    bool
    has_value() const override
    {
        return container_.has_value() && index_.has_value();
    }
    value_type const&
    read() const override
    {
        return invoker_(container_.read(), index_.read());
    }
    value_type&
    durable_ref() const override
    {
        if constexpr (
            subscript_returns_reference<
                typename ContainerSignal::value_type,
                typename IndexSignal::value_type>)
        {
            return container_.durable_ref()[index_.read()];
        }
        else
        {
            // The signal capabilities system should prevent us from ever
            // getting here.
            // LCOV_EXCL_START
            throw nullptr;
            // LCOV_EXCL_STOP
        }
    }
    decltype(auto)
    value_id() const
    {
        if constexpr (identifiable<value_type>)
            return this->read();
        else
            return std::pair{container_.value_id(), index_.value_id()};
    }
    bool
    ready_to_write() const override
    {
        return container_.has_value() && index_.has_value()
            && container_.ready_to_write();
    }
    std::optional<value_id_type>
    post_write(alia_context* ctx, value_type x) const override
    {
        if constexpr (!sink_signal<ContainerSignal>)
            return std::nullopt;
        else if constexpr (
            subscript_returns_reference<
                typename ContainerSignal::value_type,
                typename IndexSignal::value_type>
            && signal_with<
                ContainerSignal,
                binding_caps<signal_durable, signal_writable>>)
        {
            auto index = index_.read();
            value_type* element = &container_.durable_ref()[index];
            if constexpr (identifiable<value_type>)
            {
                value_type id = x;
                post_assignment(ctx, element, std::move(x));
                (void) container_.post_mutation_commit(ctx);
                return id;
            }
            else
            {
                post_assignment(ctx, element, std::move(x));
                auto container_id = container_.post_mutation_commit(ctx);
                if (!container_id)
                    return std::nullopt;
                return value_id_type{
                    std::move(*container_id), index_.value_id()};
            }
        }
        else
        {
            if constexpr (identifiable<value_type>)
            {
                value_type id = x;
                auto new_container = typename ContainerSignal::value_type(
                    read_signal(container_));
                new_container[index_.read()] = std::move(x);
                container_.post_write(ctx, std::move(new_container));
                return id;
            }
            else
            {
                auto new_container = typename ContainerSignal::value_type(
                    read_signal(container_));
                new_container[index_.read()] = std::move(x);
                auto container_id
                    = container_.post_write(ctx, std::move(new_container));
                if (!container_id)
                    return std::nullopt;
                return value_id_type{
                    std::move(*container_id), index_.value_id()};
            }
        }
    }
    std::optional<value_id_type>
    post_mutation_commit(alia_context* ctx) const override
    {
        if constexpr (
            subscript_returns_reference<
                typename ContainerSignal::value_type,
                typename IndexSignal::value_type>
            && signal_with<
                ContainerSignal,
                binding_caps<signal_durable, signal_writable>>)
        {
            auto container_id = container_.post_mutation_commit(ctx);
            if constexpr (identifiable<value_type>)
            {
                (void) container_id;
                return std::nullopt;
            }
            else
            {
                if (!container_id)
                    return std::nullopt;
                return value_id_type{
                    std::move(*container_id), index_.value_id()};
            }
        }
        else
        {
            return std::nullopt;
        }
    }

 private:
    ContainerSignal container_;
    IndexSignal index_;
    const_subscript_invoker<
        typename ContainerSignal::value_type,
        typename IndexSignal::value_type>
        invoker_;
};

template<class ContainerSignal, class IndexSignal>
subscript_signal<ContainerSignal, IndexSignal>
make_subscript_signal(ContainerSignal container, IndexSignal index)
{
    return subscript_signal<ContainerSignal, IndexSignal>(
        std::move(container), std::move(index));
}

template<class Derived, class Value, class Capabilities, class ValueId>
template<class Index>
auto
signal_base<Derived, Value, Capabilities, ValueId>::operator[](
    Index index) const
{
    return make_subscript_signal(
        static_cast<Derived const&>(*this), signalize(std::move(index)));
}

} // namespace alia
