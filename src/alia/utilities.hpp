#ifndef ALIA_UTILITIES_HPP
#define ALIA_UTILITIES_HPP

#include <alia/context.hpp>
#include <alia/flow/data_graph.hpp>

namespace alia {

// make_returnable_ref(ctx, x) stores a copy of x within the data graph of ctx
// and returns a reference to that copy. (It will move instead of copying when
// possible.)
template<class T>
T*
make_returnable_ref(context ctx, T x)
{
    T* storage;
    get_cached_data(ctx, &storage);
    *storage = std::move(x);
    return *storage;
}

} // namespace alia

#endif
