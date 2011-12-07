#ifndef ALIA_MATRIX_HPP
#define ALIA_MATRIX_HPP

#include <cassert>
#include <boost/static_assert.hpp>
#include <alia/vector.hpp>
#include <limits>
#include <algorithm>

namespace alia {

// M x N matrix class - T is the type of the scalar elements, M is the number
// of rows, and N is the number of columns.
template<unsigned M, unsigned N, typename T>
class matrix
{
 public:
    matrix() {}

    // construct from a matrix of the same size but a different type
    template<typename OtherT>
    explicit matrix(matrix<M,N,OtherT> const& other)
    {
        typename matrix<M,N,OtherT>::const_iterator j = other.begin();
        iterator end = this->end();
        for (iterator i = begin(); i != end; ++i, ++j)
            *i = static_cast<T>(*j);
    }

    // 4x4 constructors
    matrix(T v00, T v01, T v02, T v03,
           T v10, T v11, T v12, T v13,
           T v20, T v21, T v22, T v23,
           T v30, T v31, T v32, T v33)
    {
        set(v00, v01, v02, v03,
            v10, v11, v12, v13,
            v20, v21, v22, v23,
            v30, v31, v32, v33);
    }
    matrix(vector<4,T> const& column0, vector<4,T> const& column1,
        vector<4,T> const& column2, vector<4,T> const& column3)
    {
        set(column0, column1, column2, column3);
    }
    matrix(matrix<3,3,T> const& rotation, vector<3,T> const& translation)
    {
        set(rotation, translation);
    }

    // set new values (4x4)
    void set(T v00, T v01, T v02, T v03,
             T v10, T v11, T v12, T v13,
             T v20, T v21, T v22, T v23,
             T v30, T v31, T v32, T v33)
    {
        BOOST_STATIC_ASSERT(M == 4 && N == 4);
        (*this)(0, 0) = v00;
        (*this)(0, 1) = v01;
        (*this)(0, 2) = v02;
        (*this)(0, 3) = v03;
        (*this)(1, 0) = v10;
        (*this)(1, 1) = v11;
        (*this)(1, 2) = v12;
        (*this)(1, 3) = v13;
        (*this)(2, 0) = v20;
        (*this)(2, 1) = v21;
        (*this)(2, 2) = v22;
        (*this)(2, 3) = v23;
        (*this)(3, 0) = v30;
        (*this)(3, 1) = v31;
        (*this)(3, 2) = v32;
        (*this)(3, 3) = v33;
    }
    void set(vector<4,T> const& column0, vector<4,T> const& column1,
        vector<4,T> const& column2, vector<4,T> const& column3)
    {
        BOOST_STATIC_ASSERT(M == 4 && N == 4);
        (*this)(0, 0) = column0[0];
        (*this)(0, 1) = column1[0];
        (*this)(0, 2) = column2[0];
        (*this)(0, 3) = column3[0];
        (*this)(1, 0) = column0[1];
        (*this)(1, 1) = column1[1];
        (*this)(1, 2) = column2[1];
        (*this)(1, 3) = column3[1];
        (*this)(2, 0) = column0[2];
        (*this)(2, 1) = column1[2];
        (*this)(2, 2) = column2[2];
        (*this)(2, 3) = column3[2];
        (*this)(3, 0) = column0[3];
        (*this)(3, 1) = column1[3];
        (*this)(3, 2) = column2[3];
        (*this)(3, 3) = column3[3];
    }
    void set(matrix<3,3,T> const& rotation, vector<3,T> const& translation)
    {
        BOOST_STATIC_ASSERT(M == 4 && N == 4);
        (*this)(0, 0) = rotation(0, 0);
        (*this)(0, 1) = rotation(0, 1);
        (*this)(0, 2) = rotation(0, 2);
        (*this)(0, 3) = translation[0];
        (*this)(1, 0) = rotation(1, 0);
        (*this)(1, 1) = rotation(1, 1);
        (*this)(1, 2) = rotation(1, 2);
        (*this)(1, 3) = translation[1];
        (*this)(2, 0) = rotation(2, 0);
        (*this)(2, 1) = rotation(2, 1);
        (*this)(2, 2) = rotation(2, 2);
        (*this)(2, 3) = translation[2];
        (*this)(3, 0) = 0;
        (*this)(3, 1) = 0;
        (*this)(3, 2) = 0;
        (*this)(3, 3) = 1;
    }

    // 3x3 constructors
    matrix(T v00, T v01, T v02,
           T v10, T v11, T v12,
	   T v20, T v21, T v22)
    {
        set(v00, v01, v02,
            v10, v11, v12,
            v20, v21, v22);
    }
    matrix(vector<3,T> const& column0, vector<3,T> const& column1,
        vector<3,T> const& column2)
    {
        set(column0, column1, column2);
    }

    // set new values (3x3)
    void set(T v00, T v01, T v02,
             T v10, T v11, T v12,
             T v20, T v21, T v22)
    {
        BOOST_STATIC_ASSERT(M == 3 && N == 3);
        (*this)(0, 0) = v00;
        (*this)(0, 1) = v01;
        (*this)(0, 2) = v02;
        (*this)(1, 0) = v10;
        (*this)(1, 1) = v11;
        (*this)(1, 2) = v12;
        (*this)(2, 0) = v20;
        (*this)(2, 1) = v21;
        (*this)(2, 2) = v22;
    }
    void set(vector<3,T> const& column0, vector<3,T> const& column1,
        vector<3,T> const& column2)
    {
        BOOST_STATIC_ASSERT(M == 3 && N == 3);
        (*this)(0, 0) = column0[0];
        (*this)(0, 1) = column1[0];
        (*this)(0, 2) = column2[0];
        (*this)(1, 0) = column0[1];
        (*this)(1, 1) = column1[1];
        (*this)(1, 2) = column2[1];
        (*this)(2, 0) = column0[2];
        (*this)(2, 1) = column1[2];
        (*this)(2, 2) = column2[2];
    }

    // 2x2 constructors
    matrix(T v00, T v01,
           T v10, T v11)
    {
        set(v00, v01,
            v10, v11);
    }
    matrix(vector<2,T> const& column0, vector<2,T> const& column1)
    {
        set(column0, column1);
    }

    // set new values (2x2)
    void set(T v00, T v01,
             T v10, T v11)
    {
        BOOST_STATIC_ASSERT(M == 2 && N == 2);
        (*this)(0, 0) = v00;
        (*this)(0, 1) = v01;
        (*this)(1, 0) = v10;
        (*this)(1, 1) = v11;
    }
    void set(vector<2,T> const& column0, vector<2,T> const& column1)
    {
        BOOST_STATIC_ASSERT(M == 2 && N == 2);
        (*this)(0, 0) = column0[0];
        (*this)(0, 1) = column1[0];
        (*this)(1, 0) = column0[1];
        (*this)(1, 1) = column1[1];
    }

    // access individual elements
    T const& operator()(unsigned row, unsigned column) const
    {
        assert(row < M && column < N);
        return data_[row * N + column];
    }
    T& operator()(unsigned row, unsigned column)
    {
        assert(row < M && column < N);
        return data_[row * N + column];
    }

    // iterator interface
    typedef T* iterator;
    iterator begin() { return data_; }
    iterator end() { return data_ + M * N; }

    // const iterator interface
    typedef T const* const_iterator;
    const_iterator begin() const { return data_; }
    const_iterator end() const { return data_ + M * N; }

    // scalar *
    matrix& operator*=(T s)
    {
        iterator end = this->end();
        for (iterator i = begin(); i != end; ++i)
            *i *= s;
        return *this;
    }
    matrix operator*(T s) const
    {
        matrix r = (*this);
        r *= s;
        return r;
    }
    // scalar /
    matrix& operator/=(T s)
    {
        iterator end = this->end();
        for (iterator i = begin(); i != end; ++i)
            *i /= s;
        return *this;
    }
    matrix operator/(T s) const
    {
        matrix r = (*this);
        r /= s;
        return r;
    }

    // componentwise +
    matrix& operator+=(matrix const& other)
    {
        const_iterator j = other.begin();
        iterator end = this->end();
        for (iterator i = begin(); i != end; ++i, ++j)
            *i += *j;
        return *this;
    }
    matrix operator+(matrix const& other) const
    {
        matrix r = (*this);
        r += other;
        return r;
    }
    // componentwise -
    matrix& operator-=(matrix const& other)
    {
        const_iterator j = other.begin();
        iterator end = this->end();
        for (iterator i = begin(); i != end; ++i, ++j)
            *i -= *j;
        return *this;
    }
    matrix operator-(matrix const& other) const
    {
        matrix r = (*this);
        r -= other;
        return r;
    }

    // comparisons
    bool operator==(matrix const& other) const
    {
        return std::mismatch(begin(), end(), other.begin()).first == end();
    }
    bool operator!=(matrix const& other) const
    {
        return !(*this == other);
    }
    bool operator<(matrix const& other) const
    {
        for (unsigned i = 0; i != M * N; ++i)
        {
            if (data_[i] < other.data_[i])
                return true;
            else if (data_[i] > other.data_[i])
                return false;
        }
        return false;
    }

    // get the ith column as a vector
    vector<M,T> get_column(unsigned i) const
    {
        vector<M,T> v;
        for (unsigned j = 0; j < M; ++j)
        {
            v[j] = (*this)(j, i);
        }
        return v;
    }

    // set the ith column using a vector
    void set_column(unsigned i, vector<M,T> const& v)
    {
        for (unsigned j = 0; j < M; ++j)
            (*this)(j, i) = v[j];
    }

    // *= operator
    matrix<M,N,T>& operator*=(matrix<N,N,T> const& other)
    {
        *this = *this * other;
        return *this;
    }

 private:
    T data_[M * N];
};

template<unsigned M, unsigned N, typename T, typename Scalar>
matrix<M,N,T> operator*(Scalar s, matrix<M,N,T> const& m)
{
    matrix<M,N,T> r = m;
    r *= s;
    return r;
}

template <typename T, unsigned M, unsigned N, unsigned O>
matrix<M,O,T> operator*(matrix<M,N,T> const& a, matrix<N,O,T> const& b)
{
    matrix<M,O,T> r;
    for (unsigned i = 0; i < M; i++)
    {
        for (unsigned j = 0; j < O; j++)
        {
            r(i, j) = 0;
            for (unsigned k = 0; k < N; k++)
                r(i, j) += a(i, k) * b(k, j);
        }
    }
    return r;
}

// Set the given matrix to the identity.
template<unsigned N, typename T>
void load_identity(matrix<N,N,T>& m)
{
    typename matrix<N,N,T>::iterator p = m.begin();
    for (unsigned i = 0; i < N; ++i)
        for (unsigned j = 0; j < N; ++j, ++p)
            *p = static_cast<T>((i == j) ? 1 : 0);
}

// Get the identity matrix.
template<unsigned N, typename T>
matrix<N,N,T> identity_matrix()
{
    matrix<N,N,T> m;
    load_identity(m);
    return m;
}

// Get the inverse of the given matrix.
// 3x3 case
template<typename T>
matrix<3,3,T> inverse(matrix<3,3,T> const& m)
{
    matrix<3,3,T> inv(
        m(1,1) * m(2,2) - m(1,2) * m(2,1),
        m(0,2) * m(2,1) - m(0,1) * m(2,2),
        m(0,1) * m(1,2) - m(0,2) * m(1,1),
        m(1,2) * m(2,0) - m(1,0) * m(2,2),
        m(0,0) * m(2,2) - m(0,2) * m(2,0),
        m(0,2) * m(1,0) - m(0,0) * m(1,2),
        m(1,0) * m(2,1) - m(1,1) * m(2,0),
        m(0,1) * m(2,0) - m(0,0) * m(2,1),
        m(0,0) * m(1,1) - m(0,1) * m(1,0)
    );
    T det = m(0,0) * inv(0,0) + m(0,1) * inv(1,0) + m(0,2) * inv(2,0);
    if (det == 0)
        return matrix<3,3,T>(0, 0, 0, 0, 0, 0, 0, 0, 0);
    inv *= T(1) / det;
    return inv;
}
// 2x2 case
template<typename T>
matrix<2,2,T> inverse(matrix<2,2,T> const& m)
{
    T det = m(0,0) * m(1,1) - m(0,1) * m(1,0);
    if (det == 0)
        return matrix<2,2,T>(0, 0, 0, 0);
    T inv_det = static_cast<T>(1.0) / det;
    matrix<2,2,T> inv(m(1,1), -m(0,1), -m(1,0), m(0,0));
    inv *= inv_det;
    return inv;
}

}

#endif
