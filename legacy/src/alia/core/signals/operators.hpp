#ifndef ALIA_CORE_SIGNALS_OPERATORS_HPP
#define ALIA_CORE_SIGNALS_OPERATORS_HPP

#include <alia/core/signals/adaptors.hpp>
#include <alia/core/signals/application.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/utilities.hpp>

// This file defines the operators for signals.

namespace alia {

// has_value_type<T>::value yields a compile-time boolean indicating whether or
// not T has a value_type member (which is the case for standard containers).
template<class T, class = std::void_t<>>
struct has_value_type : std::false_type
{
};
template<class T>
struct has_value_type<T, std::void_t<typename T::value_type>> : std::true_type
{
};

// has_mapped_type<T>::value yields a compile-time boolean indicating whether
// or not T has a mapped_type member (which is the case for standard
// associative containers, or at least the ones that aren't sets).
template<class T, class = std::void_t<>>
struct has_mapped_type : std::false_type
{
};
template<class T>
struct has_mapped_type<T, std::void_t<typename T::mapped_type>>
    : std::true_type
{
};

// subscript_result_type<Container, Index>::type gives the expected type of the
// value that results from invoking the subscript operator on a Container.
// (This is necessary to deal with containers that return proxies.)
//
// The logic is as follows:
// 1 - If the container has a mapped_type field, use that.
// 2 - Otherwise, if the container has a value_type field, use that.
// 3 - Otherwise, just see what operator[] returns.
//
template<class Container, class Index, class = void>
struct subscript_result_type
{
};
template<class Container, class Index>
struct subscript_result_type<
    Container,
    Index,
    std::enable_if_t<has_mapped_type<Container>::value>>
{
    typedef typename Container::mapped_type type;
};
template<class Container, class Index>
struct subscript_result_type<
    Container,
    Index,
    std::enable_if_t<
        !has_mapped_type<Container>::value
        && has_value_type<Container>::value>>
{
    typedef typename Container::value_type type;
};
template<class Container, class Index>
struct subscript_result_type<
    Container,
    Index,
    std::enable_if_t<
        !has_mapped_type<Container>::value
        && !has_value_type<Container>::value>>
{
    typedef std::decay_t<
        decltype(std::declval<Container>()[std::declval<Index>()])>
        type;
};

// subscript_returns_reference<Container,Index>::value yields a
// compile-time boolean indicating whether or not the subscript operator
// for a type returns by reference (vs by value).
template<class Container, class Index>
struct subscript_returns_reference
    : std::is_reference<
          decltype(std::declval<Container>()[std::declval<Index>()])>
{
};

// has_at_indexer<Container, Index>::value yields a compile-time boolean
// indicating whether or not Container has an 'at' member function that takes
// an Index.
template<class Container, class Index, class = std::void_t<>>
struct has_at_indexer : std::false_type
{
};
template<class Container, class Index>
struct has_at_indexer<
    Container,
    Index,
    std::void_t<decltype(std::declval<Container const&>().at(
        std::declval<Index>()))>> : std::true_type
{
};

template<class Container, class Index>
auto
invoke_const_subscript(
    Container const& container,
    Index const& index,
    std::enable_if_t<!has_at_indexer<Container, Index>::value>* = 0)
    -> decltype(container[index])
{
    return container[index];
}

template<class Container, class Index>
auto
invoke_const_subscript(
    Container const& container,
    Index const& index,
    std::enable_if_t<has_at_indexer<Container, Index>::value>* = 0)
    -> decltype(container.at(index))
{
    return container.at(index);
}

// const_subscript_returns_reference<Container,Index>::value yields a
// compile-time boolean indicating whether or not invoke_const_subscript
// returns by reference (vs by value).
template<class Container, class Index>
struct const_subscript_returns_reference
    : std::is_reference<decltype(invoke_const_subscript(
          std::declval<Container>(), std::declval<Index>()))>
{
};

template<class Container, class Index, class = void>
struct const_subscript_invoker
{
};

template<class Container, class Index>
struct const_subscript_invoker<
    Container,
    Index,
    std::enable_if_t<
        const_subscript_returns_reference<Container, Index>::value>>
{
    auto const&
    operator()(Container const& container, Index const& index) const
    {
        return invoke_const_subscript(container, index);
    }
};

template<class Container, class Index>
struct const_subscript_invoker<
    Container,
    Index,
    std::enable_if_t<
        !const_subscript_returns_reference<Container, Index>::value>>
{
    auto const&
    operator()(Container const& container, Index const& index) const
    {
        storage_ = invoke_const_subscript(container, index);
        return storage_;
    }

 private:
    mutable typename subscript_result_type<Container, Index>::type storage_;
};

template<class ContainerSignal, class IndexSignal, class Value>
std::enable_if_t<signal_is_writable<ContainerSignal>::value>
write_subscript(
    ContainerSignal const& container, IndexSignal const& index, Value value)
{
    auto new_container = forward_signal(alia::move(container));
    new_container[index.read()] = std::move(value);
    container.write(std::move(new_container));
}

template<class ContainerSignal, class IndexSignal, class Value>
std::enable_if_t<!signal_is_writable<ContainerSignal>::value>
write_subscript(ContainerSignal const&, IndexSignal const&, Value)
{
}

template<class ContainerSignal, class IndexSignal>
struct subscript_signal
    : preferred_id_signal<
          subscript_signal<ContainerSignal, IndexSignal>,
          typename subscript_result_type<
              typename ContainerSignal::value_type,
              typename IndexSignal::value_type>::type,
          typename signal_capabilities_intersection<
              typename ContainerSignal::capabilities,
              typename std::conditional<
                  subscript_returns_reference<
                      typename ContainerSignal::value_type,
                      typename IndexSignal::value_type>::value,
                  movable_duplex_signal,
                  move_activated_duplex_signal>::type>::type,
          id_pair<alia::id_ref, alia::id_ref>>
{
    subscript_signal()
    {
    }
    subscript_signal(ContainerSignal array, IndexSignal index)
        : container_(std::move(array)), index_(std::move(index))
    {
    }
    bool
    has_value() const override
    {
        return container_.has_value() && index_.has_value();
    }
    typename subscript_signal::value_type const&
    read() const override
    {
        return invoker_(container_.read(), index_.read());
    }
    typename subscript_signal::value_type
    move_out() const override
    {
        auto& container = container_.destructive_ref();
        typename subscript_signal::value_type moved_out
            = std::move(container[index_.read()]);
        return moved_out;
    }
    typename subscript_signal::value_type&
    destructive_ref() const override
    {
        if constexpr (subscript_returns_reference<
                          typename ContainerSignal::value_type,
                          typename IndexSignal::value_type>::value)
        {
            auto& container = container_.destructive_ref();
            return container[index_.read()];
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
    auto
    complex_value_id() const
    {
        return combine_ids(ref(container_.value_id()), ref(index_.value_id()));
    }
    bool
    ready_to_write() const override
    {
        return container_.has_value() && index_.has_value()
            && container_.ready_to_write();
    }
    id_interface const&
    write(typename subscript_signal::value_type x) const override
    {
        write_subscript(container_, index_, std::move(x));
        return null_id;
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

template<class Derived, class Value, class Capabilities>
template<class Index>
auto
signal_base<Derived, Value, Capabilities>::operator[](Index index) const
{
    return make_subscript_signal(
        static_cast<Derived const&>(*this), signalize(std::move(index)));
}

} // namespace alia

#endif
