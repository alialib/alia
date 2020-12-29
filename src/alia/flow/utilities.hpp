#ifndef ALIA_FLOW_UTILITIES_HPP
#define ALIA_FLOW_UTILITIES_HPP

#include <alia/context/interface.hpp>
#include <alia/flow/data_graph.hpp>

namespace alia {

// make_returnable_ref(ctx, x) stores a copy of x within the data graph of ctx
// and returns a reference to that copy. (It will move instead of copying when
// possible.)

template<class T>
struct returnable_ref_node : data_node
{
    // TODO: Use optional instead.
    std::unique_ptr<T> value;

    void
    clear_cache()
    {
        value.reset();
    }
};

template<class T>
T&
make_returnable_ref(context ctx, T x)
{
    returnable_ref_node<T>* node;
    if (get_data_node(ctx, &node))
        node->value.reset(new T(x));
    else
        *node->value = std::move(x);
    return *node->value;
}

} // namespace alia

#endif