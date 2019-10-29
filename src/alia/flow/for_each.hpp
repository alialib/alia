#ifndef ALIA_FLOW_FOR_EACH_HPP
#define ALIA_FLOW_FOR_EACH_HPP

#include <alia/flow/data_graph.hpp>
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
get_alia_id(Item const& item)
{
    return no_id;
}

template<
    class Context,
    class ContainerSignal,
    class Fn,
    std::enable_if_t<
        !is_map_like<typename ContainerSignal::value_type>::value
            && is_vector_like<typename ContainerSignal::value_type>::value,
        int> = 0>
void
for_each(Context ctx, ContainerSignal const& container_signal, Fn const& fn)
{
    ALIA_IF(is_readable(container_signal))
    {
        naming_context nc(ctx);
        auto const& container = read(container_signal);
        size_t const item_count = container.size();
        for (size_t index = 0; index != item_count; ++index)
        {
            named_block nb;
            auto iteration_id = get_alia_id(container[index]);
            if (iteration_id != no_id)
                nb.begin(nc, iteration_id);
            else
                nb.begin(nc, make_id(index));
            fn(ctx, container_signal[value(index)]);
        }
    }
    ALIA_END
}

} // namespace alia

#endif
