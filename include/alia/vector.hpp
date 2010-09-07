#ifndef ALIA_VECTOR_HPP
#define ALIA_VECTOR_HPP

#include <alia/forward.hpp>
#include <alia/geometric_tuple.hpp>
#include <cmath>

namespace alia {

template<unsigned N, class T>
struct vector : geometric_tuple<N,T>
{
    vector() {}

    // construction from values
    explicit vector(T v0)
    {
        BOOST_STATIC_ASSERT(N == 1);
        this->template get<0>() = v0;
    }
    vector(T v0, T v1)
    {
        BOOST_STATIC_ASSERT(N == 2);
        this->template get<0>() = v0;
        this->template get<1>() = v1;
    }
    vector(T v0, T v1, T v2)
    {
        BOOST_STATIC_ASSERT(N == 3);
        this->template get<0>() = v0;
        this->template get<1>() = v1;
        this->template get<2>() = v2;
    }
    vector(T v0, T v1, T v2, T v3)
    {
        BOOST_STATIC_ASSERT(N == 4);
        this->template get<0>() = v0;
        this->template get<1>() = v1;
        this->template get<2>() = v2;
        this->template get<3>() = v3;
    }

    // explicit construction from other tuples
    template<class OtherT>
    explicit vector(geometric_tuple<N,OtherT> const& other)
    {
        for (unsigned i = 0; i < N; i++)
            (*this)[i] = static_cast<T>(other[i]);
    }
};

// componentwise arithmetic assignment operators
#define CRADLE_COMPONENT_OPERATOR(op) \
    template<unsigned N, class T> \
    vector<N,T>& operator op(vector<N,T>& a, vector<N,T> const& b) \
    { \
        for (unsigned i = 0; i < N; ++i) \
            a[i] op b[i]; \
        return a; \
    }
CRADLE_COMPONENT_OPERATOR(+=)
CRADLE_COMPONENT_OPERATOR(-=)
CRADLE_COMPONENT_OPERATOR(*=)
CRADLE_COMPONENT_OPERATOR(/=)
#undef CRADLE_COMPONENT_OPERATOR

// scalar arithmetic assignment operators
#define CRADLE_SCALAR_OPERATOR(op) \
    template<unsigned N, class T, class Scalar> \
    vector<N,T>& operator op(vector<N,T>& a, Scalar b) \
    { \
        for (unsigned i = 0; i < N; ++i) \
            a[i] op b; \
        return a; \
    }
CRADLE_SCALAR_OPERATOR(*=)
CRADLE_SCALAR_OPERATOR(/=)
CRADLE_SCALAR_OPERATOR(%=)
#undef CRADLE_SCALAR_OPERATOR

// componentwise operators
#define CRADLE_COMPONENT_OPERATOR(op) \
    template<unsigned N, class T> \
    vector<N,T> operator op(vector<N,T> const& a, vector<N,T> const& b) \
    { \
        vector<N,T> r; \
        for (unsigned i = 0; i < N; ++i) \
            r[i] = a[i] op b[i]; \
        return r; \
    }
CRADLE_COMPONENT_OPERATOR(+)
CRADLE_COMPONENT_OPERATOR(-)
CRADLE_COMPONENT_OPERATOR(*)
CRADLE_COMPONENT_OPERATOR(/)
#undef CRADLE_COMPONENT_OPERATOR

// scalar operators
#define CRADLE_COMMUTATIVE_SCALAR_OPERATOR(op) \
    template<unsigned N, class T, class Scalar> \
    vector<N,T> operator op(vector<N,T> const& a, Scalar b) \
    { \
        vector<N,T> r; \
        for (unsigned i = 0; i < N; ++i) \
            r[i] = a[i] op b; \
        return r; \
    } \
    template<unsigned N, class T, class Scalar> \
    vector<N,T> operator op(Scalar a, vector<N,T> const& b) \
    { \
        vector<N,T> r; \
        for (unsigned i = 0; i < N; ++i) \
            r[i] = a op b[i]; \
        return r; \
    }
#define CRADLE_SCALAR_OPERATOR(op) \
    template<unsigned N, class T, class Scalar> \
    vector<N,T> operator op(vector<N,T> const& a, Scalar b) \
    { \
        vector<N,T> r; \
        for (unsigned i = 0; i < N; ++i) \
            r[i] = a[i] op b; \
        return r; \
    }
CRADLE_COMMUTATIVE_SCALAR_OPERATOR(*)
CRADLE_SCALAR_OPERATOR(/)
CRADLE_SCALAR_OPERATOR(%)
#undef CRADLE_COMMUTATIVE_SCALAR_OPERATOR
#undef CRADLE_SCALAR_OPERATOR

// unary negation
template<unsigned N, class T>
vector<N,T> operator-(vector<N,T> const& v)
{
    vector<N,T> r;
    for (unsigned i = 0; i < N; ++i)
        r[i] = -v[i];
    return r;
}

// equality operators
namespace impl {
    template<unsigned N, class T, unsigned I>
    struct vector_equality_test
    {
        static bool apply(vector<N,T> const& a, vector<N,T> const& b)
        {
            return a.template get<I-1>() == b.template get<I-1>() &&
                vector_equality_test<N,T,I-1>::apply(a, b);
        }
    };
    template<unsigned N, class T>
    struct vector_equality_test<N,T,0>
    {
        static bool apply(vector<N,T> const& a, vector<N,T> const& b)
        { return true; }
    };
}
template<unsigned N, class T>
bool operator==(vector<N,T> const& a, vector<N,T> const& b)
{ return impl::vector_equality_test<N,T,N>::apply(a, b); }
template<unsigned N, class T>
bool operator!=(vector<N,T> const& a, vector<N,T> const& b)
{ return !(a == b); }

// 3D cross product of two vectors
// This version stores the result in its first argument.
template<class T>
void cross(vector<3,T>* result, vector<3,T> const& a, vector<3,T> const& b)
{
    (*result)[0] = a[1] * b[2] - a[2] * b[1];
    (*result)[1] = a[2] * b[0] - a[0] * b[2];
    (*result)[2] = a[0] * b[1] - a[1] * b[0];
}
// This version returns the result.
template<class T>
vector<3,T> cross(vector<3,T> const& a, vector<3,T> const& b)
{
    vector<3,T> result;
    cross(&result, a, b);
    return result;
}

// dot product of two vectors
template<unsigned N, class T>
T dot(vector<N,T> const& a, vector<N,T> const& b)
{
    T result = 0;
    for (unsigned i = 0; i < N; ++i)
        result += a[i] * b[i];
    return result;
}

// vector length
template<unsigned N, class T>
T length(vector<N,T> const& v)
{
    return std::sqrt(length2(v));
}

// vector length squared - This is more efficient, and it's sufficient if
// you're using the length for comparison purposes.
template<unsigned N, class T>
T length2(vector<N,T> const& v)
{
    return dot(v, v);
}

// Get the unit-length version of the given vector.
template<unsigned N, class T>
vector<N,T> unit(vector<N,T> const& v)
{
    return v / length(v);
}

// Get a unit vector that's perpendicular to the given one.
template<class T>
vector<3,T> get_perpendicular(vector<3,T> const& v)
{
    vector<3,T> u;
    if ((std::abs)(v[0]) < (std::abs)(v[1]))
    {
        if ((std::abs)(v[0]) < (std::abs)(v[2]))
            u = vector<3,T>(1, 0, 0);
        else
            u = vector<3,T>(0, 0, 1);
    }
    else
    {
        if ((std::abs)(v[1]) < (std::abs)(v[2]))
            u = vector<3,T>(0, 1, 0);
        else
            u = vector<3,T>(0, 0, 1);
    }
    return unit(cross(u, v));
}

namespace impl {
    template<unsigned N, class T, unsigned Begin, unsigned Count>
    struct product_calculation
    {
        static T apply(vector<N,T> const& v)
        {
            return v[Begin+Count-1] *
                product_calculation<N,T,Begin,Count-1>::apply(v);
        }
    };
    template<unsigned N, class T, unsigned Begin>
    struct product_calculation<N,T,Begin,0>
    {
        static T apply(vector<N,T> const& v)
        { return 1; }
    };
}

// Get the product of all components within the given vector.
template<unsigned N, class T>
T product(vector<N,T> const& v)
{
    return impl::product_calculation<N,T,0,N>::apply(v);
}
// Get the product of the components whose indices lie within the range
// [Begin,End) in the given vector.
template<unsigned Begin, unsigned End, unsigned N, class T>
T partial_product(vector<N,T> const& v)
{
    BOOST_STATIC_ASSERT(End >= Begin && End <= N);
    return impl::product_calculation<N,T,Begin,End-Begin>::apply(v);
}

// Given a vector, this returns a corresponding vector with one less dimension
// by removing the value at index i.
template<unsigned N, class T>
vector<N - 1,T> slice(vector<N,T> const& p, unsigned i)
{
    assert(i < N);
    vector<N - 1,T> r;
    for(unsigned j = 0; j < i; ++j)
        r[j] = p[j];
    for(unsigned j = i; j < N - 1; ++j)
        r[j] = p[j + 1];
    return r;
}

// Given a vector, this returns a corresponding vector with one more dimension
// by inserting the given value at index i.
template<unsigned N, class T, class OtherValue>
vector<N+1,T> unslice(vector<N,T> const& p, unsigned i, OtherValue value)
{
    assert(i <= N);
    vector<N+1,T> r;
    for(unsigned j = 0; j < i; ++j)
        r[j] = p[j];
    r[i] = static_cast<T>(value);
    for(unsigned j = i; j < N; ++j)
        r[j + 1] = p[j];
    return r;
}

// Create a vector whose components all have the same value.
template<unsigned N, class T>
vector<N,T> uniform_vector(T value)
{
    vector<N,T> r;
    for (unsigned i = 0; i < N; ++i)
        r[i] = value;
    return r;
}

}

#endif
