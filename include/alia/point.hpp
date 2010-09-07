#ifndef ALIA_POINT_HPP
#define ALIA_POINT_HPP

#include <alia/vector.hpp>

namespace alia {

template<unsigned N, class T>
struct point : geometric_tuple<N,T>
{
    point() {}

    // construction from values
    explicit point(T v0)
    {
        BOOST_STATIC_ASSERT(N == 1);
        this->template get<0>() = v0;
    }
    point(T v0, T v1)
    {
        BOOST_STATIC_ASSERT(N == 2);
        this->template get<0>() = v0;
        this->template get<1>() = v1;
    }
    point(T v0, T v1, T v2)
    {
        BOOST_STATIC_ASSERT(N == 3);
        this->template get<0>() = v0;
        this->template get<1>() = v1;
        this->template get<2>() = v2;
    }
    point(T v0, T v1, T v2, T v3)
    {
        BOOST_STATIC_ASSERT(N == 4);
        this->template get<0>() = v0;
        this->template get<1>() = v1;
        this->template get<2>() = v2;
        this->template get<3>() = v3;
    }

    // explicit construction from other tuples
    template<class OtherT>
    explicit point(geometric_tuple<N,OtherT> const& other)
    {
        for (unsigned i = 0; i < N; i++)
            (*this)[i] = static_cast<T>(other[i]);
    }
};

// componentwise arithmetic operators
template<unsigned N, class T>
point<N,T>& operator+=(point<N,T>& p, vector<N,T> const& v)
{
    for (unsigned i = 0; i < N; ++i)
        p[i] += v[i];
    return p;
}
template<unsigned N, class T>
point<N,T> operator+(point<N,T> const& p, vector<N,T> const& v)
{
    point<N,T> r;
    for (unsigned i = 0; i < N; ++i)
        r[i] = p[i] + v[i];
    return r;
}
template<unsigned N, class T>
point<N,T> operator+(vector<N,T> const& v, point<N,T> const& p)
{
    point<N,T> r;
    for (unsigned i = 0; i < N; ++i)
        r[i] = v[i] + p[i];
    return r;
}
template<unsigned N, class T>
point<N,T>& operator-=(point<N,T>& p, vector<N,T> const& v)
{
    for (unsigned i = 0; i < N; ++i)
        p[i] -= v[i];
    return p;
}
template<unsigned N, class T>
point<N,T> operator-(point<N,T> const& p, vector<N,T> const& v)
{
    point<N,T> r;
    for (unsigned i = 0; i < N; ++i)
        r[i] = p[i] - v[i];
    return r;
}
template<unsigned N, class T>
vector<N,T> operator-(point<N,T> const& p1, point<N,T> const& p2)
{
    vector<N,T> r;
    for (unsigned i = 0; i < N; ++i)
        r[i] = p1[i] - p2[i];
    return r;
}

// equality operators
namespace impl {
    template<unsigned N, class T, unsigned I>
    struct point_equality_test
    {
        static bool apply(point<N,T> const& a, point<N,T> const& b)
        {
            return a.template get<I-1>() == b.template get<I-1>() &&
                point_equality_test<N,T,I-1>::apply(a, b);
        }
    };
    template<unsigned N, class T>
    struct point_equality_test<N,T,0>
    {
        static bool apply(point<N,T> const& a, point<N,T> const& b)
        { return true; }
    };
}
template<unsigned N, class T>
bool operator==(point<N,T> const& a, point<N,T> const& b)
{ return impl::point_equality_test<N,T,N>::apply(a, b); }
template<unsigned N, class T>
bool operator!=(point<N,T> const& a, point<N,T> const& b)
{ return !(a == b); }

// Given a point, this returns a corresponding point with one less dimension
// by removing the value at index i.
template<unsigned N, class T>
point<N - 1,T> slice(point<N,T> const& p, unsigned i)
{
    assert(i < N);
    point<N-1,T> r;
    for(unsigned j = 0; j < i; ++j)
        r[j] = p[j];
    for(unsigned j = i; j < N - 1; ++j)
        r[j] = p[j + 1];
    return r;
}

// Given a point, this returns a corresponding point with one more dimension
// by inserting the given value at index i.
template<unsigned N, class T, class OtherValue>
point<N+1,T> unslice(point<N,T> const& p, unsigned i, OtherValue value)
{
    assert(i <= N);
    point<N+1,T> r;
    for(unsigned j = 0; j < i; ++j)
        r[j] = p[j];
    r[i] = static_cast<T>(value);
    for(unsigned j = i; j < N; ++j)
        r[j + 1] = p[j];
    return r;
}

// Create a point whose components all have the same value.
template<unsigned N, class T>
point<N,T> uniform_point(T value)
{
    point<N,T> r;
    for (unsigned i = 0; i < N; ++i)
        r[i] = value;
    return r;
}

}

#endif
