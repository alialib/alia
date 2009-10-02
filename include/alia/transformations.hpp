#ifndef ALIA_TRANSFORMATIONS_HPP
#define ALIA_TRANSFORMATIONS_HPP

#include <alia/point.hpp>
#include <alia/vector.hpp>
#include <alia/matrix.hpp>

namespace alia {

// A linear transformation for N-dimensional space is a represented as an
// (N+1)x(N+1) matrix where the bottom row of the matrix is (0 ... 0 1).
// This file provides several methods for creating and applying linear
// transformations of this form.
// All the transformation generators come in two forms: one that directly
// returns the resulting matrix, and one that writes it to a caller-supplied
// matrix (often more efficient, but less convenient).  The latter form is
// prefixed with 'load_'.

// SCALING

// Generate a scaling transformation.
template<unsigned N, typename T>
matrix<N + 1,N + 1,T> scaling_transformation(vector<N,T> const& scale)
{
    matrix<N + 1,N + 1,T> m;
    load_scaling_transformation(m, scale);
    return m;
}
// load_ form
template<unsigned N, typename T>
void load_scaling_transformation(matrix<N + 1,N + 1,T>& m,
    vector<N,T> const& scale)
{
    typename matrix<N + 1,N + 1,T>::iterator p = m.begin();
    for (unsigned i = 0; i < N + 1; ++i)
        for (unsigned j = 0; j < N + 1; ++j, ++p)
            *p = (i == j) ? ((i < N) ? scale[i] : 1) : 0;
}

// TRANSLATION

// Generate a translation.
template<unsigned N, typename T>
matrix<N + 1,N + 1,T> translation(vector<N,T> const& v)
{
    matrix<N + 1,N + 1,T> m;
    load_translation(m, v);
    return m;
}

// load_ form
template<unsigned N, typename T>
void load_translation(matrix<N + 1,N + 1,T>& m, vector<N,T> const& v)
{
    typename matrix<N + 1,N + 1,T>::iterator p = m.begin();
    for (unsigned i = 0; i < N + 1; ++i)
        for (unsigned j = 0; j < N + 1; ++j, ++p)
            *p = (i == j) ? 1 : ((j == N) ? v[i] : 0);
}

// ROTATION - All angles are in radians.

static const double pi = 3.1415926535897932384626433832795;

// Generate a 2D rotation.
template<typename T>
matrix<3,3,T> rotation(T a)
{
    matrix<3,3,T> m;
    load_rotation(m, a);
    return m;
}
// load_ form
template<typename T>
void load_rotation(matrix<3,3,T>& m, T a)
{
    T c = cos(a), s = sin(a);
    m.set(c,-s, 0,
	  s, c, 0,
	  0, 0, 1);
}

// Generate a 3D rotation about the X-axis.
template<typename T>
matrix<4,4,T> rotation_about_x(T a)
{
    matrix<4,4,T> m;
    load_rotation_about_x(m, a);
    return m;
}
// load_ form
template<typename T>
void load_rotation_about_x(matrix<4,4,T>& m, T a)
{
    T c = cos(a), s = sin(a);
    m.set(1, 0, 0, 0,
	  0, c,-s, 0,
	  0, s, c, 0,
	  0, 0, 0, 1);
}

// Generate a 3D rotation about the Y-axis.
template<typename T>
matrix<4,4,T> rotation_about_y(T a)
{
    matrix<4,4,T> m;
    load_rotation_about_y(m, a);
    return m;
}

// load_ form
template<typename T>
void load_rotation_about_y(matrix<4,4,T>& m, T a)
{
    T c = cos(a), s = sin(a);
    m.set(c, 0, s, 0,
	  0, 1, 0, 0,
	 -s, 0, c, 0,
	  0, 0, 0, 1);
}

// Generate a 3D rotation about the Z-axis.
template<typename T>
matrix<4,4,T> rotation_about_z(T a)
{
    matrix<4,4,T> m;
    load_rotation_about_z(m, a);
    return m;
}
// load_ form
template<typename T>
void load_rotation_about_z(matrix<4,4,T>& m, T a)
{
    T c = cos(a), s = sin(a);
    m.set(c,-s, 0, 0,
	  s, c, 0, 0,
	  0, 0, 1, 0,
	  0, 0, 0, 1);
}

// Generate a 3D rotation about an arbitrary axis.
template<typename T>
matrix<4,4,T> rotation_about_axis(vector<3,T> const& axis, T a)
{
    matrix<4,4,T> m;
    load_rotation_about_axis(m, axis, a);
    return m;
}

// load_ form
template<typename T, class AngleUnits>
void load_rotation_about_axis(matrix<4,4,T>& m, vector<3,T> const& axis, T a)
{
    T const c = cos(a);
    T const s = sin(a);
    T const omc = 1 - c;

    T const& x = axis(0);
    T const& y = axis(1);
    T const& z = axis(2);

    m.set(x * x * omc + c,     x * y * omc - z * s, x * z * omc + y * s, 0,
          y * x * omc + z * s, y * y * omc + c,     y * z * omc - x * s, 0,
	  z * x * omc - y * s, z * y * omc + x * s, z * z * omc + c,     0,
	  0,                   0,                   0,                   1);
}

// APPLYING TRANSFORMATIONS

// points

// Transform a point by a matrix and write the result to the first argument.
// 3D case
template<typename T>
void transform_point(point<3,T>* result, matrix<4,4,T> const& m,
    point<3,T> const& p)
{
    (*result)[0] = p[0] * m(0,0) + p[1] * m(0,1) + p[2] * m(0,2) + m(0,3);
    (*result)[1] = p[0] * m(1,0) + p[1] * m(1,1) + p[2] * m(1,2) + m(1,3);
    (*result)[2] = p[0] * m(2,0) + p[1] * m(2,1) + p[2] * m(2,2) + m(2,3);
    //assert(
    //    math::almost_equal<T>(m(3,0), 0) &&
    //    math::almost_equal<T>(m(3,1), 0) &&
    //    math::almost_equal<T>(m(3,2), 0) &&
    //    math::almost_equal<T>(m(3,3), 1));
}
// 2D case
template<typename T>
void transform_point(point<2,T>* result, matrix<3,3,T> const& m,
    point<2,T> const& p)
{
    (*result)[0] = p[0] * m(0,0) + p[1] * m(0,1) + m(0,2);
    (*result)[1] = p[0] * m(1,0) + p[1] * m(1,1) + m(1,2);
    //assert(
    //    math::almost_equal<T>(m(2,0), 0) &&
    //    math::almost_equal<T>(m(2,1), 0) &&
    //    math::almost_equal<T>(m(2,2), 1));
}

// Same as above but the result is returned.
// 3D case
template<typename T>
point<3,T> transform_point(matrix<4,4,T> const& m, point<3,T> const& p)
{
    point<3,T> result;
    transform_point(&result, m, p);
    return result;
}
// 2D case
template<typename T>
point<2,T> transform_point(matrix<3,3,T> const& m, point<2,T> const& p)
{
    point<2,T> result;
    transform_point(&result, m, p);
    return result;
}

// vectors

// Transform a vector by a matrix and write the result to the first argument.
// 3D case
template<typename T>
void transform_vector(vector<3,T>* result, matrix<4,4,T> const& m,
    vector<3,T> const& v)
{
    (*result)[0] = v[0] * m(0,0) + v[1] * m(0,1) + v[2] * m(0,2);
    (*result)[1] = v[0] * m(1,0) + v[1] * m(1,1) + v[2] * m(1,2);
    (*result)[2] = v[0] * m(2,0) + v[1] * m(2,1) + v[2] * m(2,2);
    //assert(
    //    math::almost_equal<T>(m(3,0), 0) &&
    //    math::almost_equal<T>(m(3,1), 0) &&
    //    math::almost_equal<T>(m(3,2), 0) &&
    //    math::almost_equal<T>(m(3,3), 1));
}

// 2D case
template<typename T>
void transform_vector(vector<2,T>* result, matrix<3,3,T> const& m,
    vector<2,T> const& v)
{
    (*result)[0] = v[0] * m(0,0) + v[1] * m(0,1);
    (*result)[1] = v[0] * m(1,0) + v[1] * m(1,1);
    //assert(
    //    math::almost_equal<T>(m(2,0), 0) &&
    //    math::almost_equal<T>(m(2,1), 0) &&
    //    math::almost_equal<T>(m(2,2), 1));
}

// Same as above but the result is returned.
// 3D case
template<typename T>
vector<3,T> transform_vector(matrix<4,4,T> const& m, vector<3,T> const& v)
{
    vector<3,T> result;
    transform_vector(&result, m, v);
    return result;
}
// 2D case
template<typename T>
vector<2,T> transform_vector(matrix<3,3,T> const& m, vector<2,T> const& v)
{
    vector<2,T> result;
    transform_vector(&result, m, v);
    return result;
}

}

#endif
