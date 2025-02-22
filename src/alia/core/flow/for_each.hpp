#ifndef ALIA_CORE_FLOW_FOR_EACH_HPP
#define ALIA_CORE_FLOW_FOR_EACH_HPP

#include <alia/core/flow/macros.hpp>
#include <alia/core/signals/adaptors.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/operators.hpp>

namespace alia {

// is_map_like<Container>::value yields a compile-time boolean indicating
// whether or not Container behaves like a map for the purposes of alia
// iteration and indexing. (This is determined by checking whether or not
// Container has both a key_type and a mapped_type member.)
template<class T, class = std::void_t<>>
struct is_map_like : std::false_type
{
};
template<class T>
struct is_map_like<
    T,
    std::void_t<typename T::key_type, typename T::mapped_type>>
    : std::true_type
{
};

// is_vector_like<Container>::value yields a compile-time boolean indicating
// whether or not Container behaves like a vector for the purposes of alia
// iteration and indexing. (This is determined by checking whether or not
// Container can be subscripted with a size_t. This is sufficient because the
// main purpose is to distinguish vector-like containers from list-like ones.)
template<class Container, class = std::void_t<>>
struct is_vector_like : std::false_type
{
};
template<class Container>
struct is_vector_like<
    Container,
    std::void_t<
        typename Container::value_type,
        typename Container::size_type,
        decltype(std::declval<Container>().at(size_t(0)))>> : std::true_type
{
};

template<class Item>
auto
get_alia_item_id(Item const&)
{
    return null_id;
}

// invoke_map_iteration_body selects the appropriate way to invoke the
// iteration body function provided to for_each (for map-like containers).
template<
    class IterationBody,
    class NamedBlockBegin,
    class Key,
    class Value,
    std::enable_if_t<
        std::is_invocable<IterationBody&&, naming_context&, Key&&, Value&&>::
            value,
        int>
    = 0>
void
invoke_map_iteration_body(
    IterationBody&& body,
    naming_context& nc,
    NamedBlockBegin&&,
    Key&& key,
    Value&& value)
{
    body(nc, key, value);
}
template<
    class IterationBody,
    class NamedBlockBegin,
    class Key,
    class Value,
    std::enable_if_t<
        std::is_invocable<IterationBody&&, Key&&, Value&&>::value,
        int>
    = 0>
void
invoke_map_iteration_body(
    IterationBody&& body,
    naming_context&,
    NamedBlockBegin&& nb_begin,
    Key&& key,
    Value&& value)
{
    named_block nb;
    nb_begin(nb);
    body(key, value);
}

// for_each for map-like containers
template<
    class Context,
    class ContainerSignal,
    class Fn,
    std::enable_if_t<
        is_signal_type<ContainerSignal>::value
            && is_map_like<typename ContainerSignal::value_type>::value,
        int>
    = 0>
void
for_each(Context ctx, ContainerSignal const& container_signal, Fn&& fn)
{
    ALIA_IF (has_value(container_signal))
    {
        naming_context nc(ctx);
        auto const& container = read_signal(container_signal);
        for (auto const& item : container)
        {
            auto key = direct(item.first);
            auto value = container_signal[key];
            invoke_map_iteration_body(
                fn,
                nc,
                [&](named_block& nb) { nb.begin(nc, make_id(item.first)); },
                key,
                value);
        }
    }
    ALIA_END
}

// invoke_sequence_iteration_body selects the appropriate way to invoke the
// iteration body function provided to for_each (for sequence containers).
template<
    class IterationBody,
    class NamedBlockBegin,
    class Item,
    std::enable_if_t<
        std::is_invocable<IterationBody&&, naming_context&, size_t, Item&&>::
            value,
        int>
    = 0>
void
invoke_sequence_iteration_body(
    IterationBody&& body,
    naming_context& nc,
    NamedBlockBegin&&,
    size_t index,
    Item&& item)
{
    body(nc, index, item);
}
template<
    class IterationBody,
    class NamedBlockBegin,
    class Item,
    std::enable_if_t<
        std::is_invocable<IterationBody&&, size_t, Item&&>::value,
        int>
    = 0>
void
invoke_sequence_iteration_body(
    IterationBody&& body,
    naming_context&,
    NamedBlockBegin&& nb_begin,
    size_t index,
    Item&& item)
{
    named_block nb;
    nb_begin(nb);
    body(index, item);
}
template<
    class IterationBody,
    class NamedBlockBegin,
    class Item,
    std::enable_if_t<
        std::is_invocable<IterationBody&&, naming_context&, Item&&>::value,
        int>
    = 0>
void
invoke_sequence_iteration_body(
    IterationBody&& body,
    naming_context& nc,
    NamedBlockBegin&&,
    size_t,
    Item&& item)
{
    body(nc, item);
}
template<
    class IterationBody,
    class NamedBlockBegin,
    class Item,
    std::enable_if_t<std::is_invocable<IterationBody&&, Item&&>::value, int>
    = 0>
void
invoke_sequence_iteration_body(
    IterationBody&& body,
    naming_context&,
    NamedBlockBegin&& nb_begin,
    size_t,
    Item&& item)
{
    named_block nb;
    nb_begin(nb);
    body(item);
}

// for_each for signals carrying vector-like containers
template<
    class Context,
    class ContainerSignal,
    class Fn,
    std::enable_if_t<
        is_signal_type<ContainerSignal>::value
            && !is_map_like<typename ContainerSignal::value_type>::value
            && is_vector_like<typename ContainerSignal::value_type>::value,
        int>
    = 0>
void
for_each(Context ctx, ContainerSignal const& container_signal, Fn&& fn)
{
    ALIA_IF (has_value(container_signal))
    {
        naming_context nc(ctx);
        auto const& container = read_signal(container_signal);
        size_t const item_count = container.size();
        for (size_t index = 0; index != item_count; ++index)
        {
            invoke_sequence_iteration_body(
                fn,
                nc,
                [&](named_block& nb) {
                    auto iteration_id = get_alia_item_id(container[index]);
                    if (iteration_id != null_id)
                        nb.begin(nc, iteration_id);
                    else
                        nb.begin(nc, make_id(index));
                },
                index,
                container_signal[value(index)]);
        }
    }
    ALIA_END
}

// for_each for vector-like containers of signals
template<
    class Context,
    class Container,
    class Fn,
    std::enable_if_t<
        !is_signal_type<
            std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && !is_map_like<
                std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && is_vector_like<
                std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && is_signal_type<typename std::remove_cv_t<
                std::remove_reference_t<Container>>::value_type>::value,
        int>
    = 0>
void
for_each(Context ctx, Container&& container, Fn&& fn)
{
    naming_context nc(ctx);
    size_t index = 0;
    for (auto&& item : container)
    {
        invoke_sequence_iteration_body(
            fn,
            nc,
            [&](named_block& nb) {
                // We don't try to use get_alia_item_id() here because we want
                // to support the use case where the UI is present even when
                // the item isn't available, and we want to keep the block ID
                // stable in that scenario.
                nb.begin(nc, make_id(index));
            },
            index,
            item);
        ++index;
    }
}

// for_each for vector-like containers of raw values
template<
    class Context,
    class Container,
    class Fn,
    std::enable_if_t<
        !is_signal_type<
            std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && !is_map_like<
                std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && is_vector_like<
                std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && !is_signal_type<typename std::remove_cv_t<
                std::remove_reference_t<Container>>::value_type>::value,
        int>
    = 0>
void
for_each(Context ctx, Container&& container, Fn&& fn)
{
    naming_context nc(ctx);
    size_t index = 0;
    for (auto&& item : container)
    {
        invoke_sequence_iteration_body(
            fn,
            nc,
            [&](named_block& nb) {
                auto iteration_id = get_alia_item_id(item);
                if (iteration_id != null_id)
                    nb.begin(nc, iteration_id);
                else
                    nb.begin(nc, make_id(index));
            },
            index,
            item);
        ++index;
    }
}

// signal type for accessing items within a list
template<class ListSignal, class Item>
struct list_item_signal
    : signal<
          list_item_signal<ListSignal, Item>,
          Item,
          typename ListSignal::capabilities>
{
    list_item_signal(ListSignal const& list_signal, size_t index, Item* item)
        : list_signal_(list_signal), index_(index), item_(item)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = combine_ids(ref(list_signal_.value_id()), make_id(index_));
        return id_;
    }
    bool
    has_value() const
    {
        return list_signal_.has_value();
    }
    Item const&
    read() const
    {
        return *item_;
    }
    Item
    move_out() const
    {
        return *item_;
    }
    Item&
    destructive_ref() const
    {
        return *item_;
    }
    bool
    ready_to_write() const
    {
        return list_signal_.ready_to_write();
    }
    id_interface const&
    write(Item value) const
    {
        *item_ = std::move(value);
        return null_id;
    }

 private:
    ListSignal list_signal_;
    size_t index_;
    Item* item_;
    mutable id_pair<id_ref, simple_id<size_t>> id_;
};
template<class ListSignal, class Item>
list_item_signal<ListSignal, Item>
make_list_item_signal(ListSignal const& signal, size_t index, Item const* item)
{
    return list_item_signal<ListSignal, Item>(
        signal, index, const_cast<Item*>(item));
}

// for_each for list-like signal containers
template<
    class Context,
    class ContainerSignal,
    class Fn,
    std::enable_if_t<
        is_signal_type<ContainerSignal>::value
            && !is_map_like<typename ContainerSignal::value_type>::value
            && !is_vector_like<typename ContainerSignal::value_type>::value,
        int>
    = 0>
void
for_each(Context ctx, ContainerSignal const& container_signal, Fn&& fn)
{
    ALIA_IF (has_value(container_signal))
    {
        naming_context nc(ctx);
        auto const& container = read_signal(container_signal);
        size_t index = 0;
        for (auto const& item : container)
        {
            invoke_sequence_iteration_body(
                fn,
                nc,
                [&](named_block& nb) {
                    auto iteration_id = get_alia_item_id(item);
                    if (iteration_id != null_id)
                        nb.begin(nc, iteration_id);
                    else
                        nb.begin(nc, make_id(&item));
                },
                index,
                make_list_item_signal(container_signal, index, &item));
            ++index;
        }
    }
    ALIA_END
}

// for_each for list-like containers of signals
template<
    class Context,
    class Container,
    class Fn,
    std::enable_if_t<
        !is_signal_type<
            std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && !is_map_like<
                std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && !is_vector_like<
                std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && is_signal_type<typename std::remove_cv_t<
                std::remove_reference_t<Container>>::value_type>::value,
        int>
    = 0>
void
for_each(Context ctx, Container&& container, Fn&& fn)
{
    naming_context nc(ctx);
    size_t index = 0;
    for (auto&& item : container)
    {
        invoke_sequence_iteration_body(
            fn,
            nc,
            [&](named_block& nb) {
                // We don't try to use get_alia_item_id() here because we want
                // to support the use case where the UI is present even when
                // the item isn't available, and we want to keep the block ID
                // stable in that scenario.
                nb.begin(nc, make_id(&item));
            },
            index,
            item);
        ++index;
    }
}

// for_each for list-like containers of raw values
template<
    class Context,
    class Container,
    class Fn,
    std::enable_if_t<
        !is_signal_type<
            std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && !is_map_like<
                std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && !is_vector_like<
                std::remove_cv_t<std::remove_reference_t<Container>>>::value
            && !is_signal_type<typename std::remove_cv_t<
                std::remove_reference_t<Container>>::value_type>::value,
        int>
    = 0>
void
for_each(Context ctx, Container&& container, Fn&& fn)
{
    naming_context nc(ctx);
    size_t index = 0;
    for (auto&& item : container)
    {
        invoke_sequence_iteration_body(
            fn,
            nc,
            [&](named_block& nb) {
                auto iteration_id = get_alia_item_id(item);
                if (iteration_id != null_id)
                    nb.begin(nc, iteration_id);
                else
                    nb.begin(nc, make_id(&item));
            },
            index,
            item);
        ++index;
    }
}

} // namespace alia

#endif
