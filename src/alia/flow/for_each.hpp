#ifndef ALIA_FLOW_FOR_EACH_HPP
#define ALIA_FLOW_FOR_EACH_HPP

#include <alia/flow/macros.hpp>
#include <alia/signals/adaptors.hpp>
#include <alia/signals/basic.hpp>
#include <alia/signals/operators.hpp>

namespace alia {

// is_map_like<Container>::value yields a compile-time boolean indicating
// whether or not Container behaves like a map for the purposes of alia
// iteration and indexing. (This is determined by checking whether or not
// Container has both a key_type and a mapped_type member.)
template<class T, class = void_t<>>
struct is_map_like : std::false_type
{
};
template<class T>
struct is_map_like<T, void_t<typename T::key_type, typename T::mapped_type>>
    : std::true_type
{
};

// is_vector_like<Container>::value yields a compile-time boolean indicating
// whether or not Container behaves like a vector for the purposes of alia
// iteration and indexing. (This is determined by checking whether or not
// Container can be subscripted with a size_t. This is sufficient because the
// main purpose is to distinguish vector-like containers from list-like ones.)
template<class Container, class = void_t<>>
struct is_vector_like : std::false_type
{
};
template<class Container>
struct is_vector_like<
    Container,
    void_t<decltype(std::declval<Container>()[size_t(0)])>> : std::true_type
{
};

template<class Item>
auto
get_alia_id(Item const&)
{
    return null_id;
}

// for_each for map-like containers
template<
    class Context,
    class ContainerSignal,
    class Fn,
    std::enable_if_t<
        is_map_like<typename ContainerSignal::value_type>::value,
        int> = 0>
void
for_each(Context ctx, ContainerSignal const& container_signal, Fn&& fn)
{
    ALIA_IF(has_value(container_signal))
    {
        naming_context nc(ctx);
        auto const& container = read_signal(container_signal);
        for (auto const& item : container)
        {
            named_block nb;
            auto iteration_id = get_alia_id(item.first);
            if (iteration_id != null_id)
            {
                nb.begin(nc, iteration_id);
            }
            else
            {
                nb.begin(nc, make_id(item.first));
            }
            auto key = direct(item.first);
            auto value = container_signal[key];
            fn(ctx, key, value);
        }
    }
    ALIA_END
}

// for_each for vector-like containers
template<
    class Context,
    class ContainerSignal,
    class Fn,
    std::enable_if_t<
        !is_map_like<typename ContainerSignal::value_type>::value
            && is_vector_like<typename ContainerSignal::value_type>::value,
        int> = 0>
void
for_each(Context ctx, ContainerSignal const& container_signal, Fn&& fn)
{
    ALIA_IF(has_value(container_signal))
    {
        naming_context nc(ctx);
        auto const& container = read_signal(container_signal);
        size_t const item_count = container.size();
        for (size_t index = 0; index != item_count; ++index)
        {
            named_block nb;
            auto iteration_id = get_alia_id(container[index]);
            if (iteration_id != null_id)
                nb.begin(nc, iteration_id);
            else
                nb.begin(nc, make_id(index));
            fn(ctx, container_signal[value(index)]);
        }
    }
    ALIA_END
}

// signal type for accessing items within a list
template<class ListSignal, class Item>
struct list_item_signal : signal<
                              list_item_signal<ListSignal, Item>,
                              Item,
                              typename ListSignal::direction_tag>
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
    movable_value() const
    {
        return *item_;
    }
    bool
    ready_to_write() const
    {
        return list_signal_.ready_to_write();
    }
    void
    write(Item value) const
    {
        *item_ = std::move(value);
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

// for_each for list-like containers
template<
    class Context,
    class ContainerSignal,
    class Fn,
    std::enable_if_t<
        !is_map_like<typename ContainerSignal::value_type>::value
            && !is_vector_like<typename ContainerSignal::value_type>::value,
        int> = 0>
void
for_each(Context ctx, ContainerSignal const& container_signal, Fn&& fn)
{
    ALIA_IF(has_value(container_signal))
    {
        naming_context nc(ctx);
        auto const& container = read_signal(container_signal);
        size_t index = 0;
        for (auto const& item : container)
        {
            named_block nb;
            auto iteration_id = get_alia_id(item);
            if (iteration_id != null_id)
                nb.begin(nc, iteration_id);
            else
                nb.begin(nc, make_id(&item));
            fn(ctx, make_list_item_signal(container_signal, index, &item));
            ++index;
        }
    }
    ALIA_END
}

} // namespace alia

#endif
