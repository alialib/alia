#ifndef ALIA_GEOMETRIC_TUPLE_HPP
#define ALIA_GEOMETRIC_TUPLE_HPP

#include <boost/static_assert.hpp>
#include <cassert>

namespace alia {

// geometric_tuple is the common base class for points and vectors
template<unsigned N, class T>
struct geometric_tuple
{
    // allow external access to template parameters
    typedef T value_type;
    static const unsigned dimensionality = N;

    // value accessors with run-time index
    T operator[](unsigned i) const
    {
        assert(i < N);
        return data[i];
    }
    T& operator[](unsigned i)
    {
        assert(i < N);
        return data[i];
    }

    // value accessors with compile-time index
    template<unsigned I>
    T get() const
    {
        BOOST_STATIC_ASSERT(I < N);
        return data[I];
    }
    template<unsigned I>
    T& get()
    {
        BOOST_STATIC_ASSERT(I < N);
        return data[I];
    }

    T data[N];
};

template<class Tuple>
struct geometric_tuple_value_access_type
{
    typedef typename Tuple::value_type& type;
};
template<class Tuple>
struct geometric_tuple_value_access_type<Tuple const>
{
    typedef typename Tuple::value_type const type;
};

}

#endif
