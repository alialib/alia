#ifndef ALIA_CORE_FLOW_UTILITIES_HPP
#define ALIA_CORE_FLOW_UTILITIES_HPP

#include <alia/core/context/interface.hpp>
#include <alia/core/flow/data_graph.hpp>

namespace alia {

// make_returnable_ref(ctx, x) stores a copy of x within the data graph of ctx
// and returns a reference to that copy. (It will move instead of copying when
// possible.)

template<class T>
struct returnable_ref_node : data_node
{
    T value;

    returnable_ref_node(T& x) : value(std::move(x))
    {
    }
};

template<class T>
T&
make_returnable_ref(context ctx, T x)
{
    returnable_ref_node<T>* node;
    if (!get_data_node(
            ctx, &node, [&] { return new returnable_ref_node<T>(x); }))
    {
        node->value = std::move(x);
    }
    return node->value;
}

} // namespace alia

#endif
