#ifndef ALIA_GEOMETRY_HPP
#define ALIA_GEOMETRY_HPP

#include <alia/common.hpp>
#include <cmath>
#include <ostream>

// This is the geometry library for alia. It's focused on the types of geometry
// that are needed by a 2D UI library: vectors, boxes, transformation matrices,
// etc.

namespace alia {

// VECTOR

// componentwise arithmetic assignment operators
#define ALIA_COMPONENT_OPERATOR(op) \
    template<unsigned N, class T> \
    vector<N,T>& operator op(vector<N,T>& a, vector<N,T> const& b) \
    { \
        for (unsigned i = 0; i < N; ++i) \
            a[i] op b[i]; \
        return a; \
    }
ALIA_COMPONENT_OPERATOR(+=)
ALIA_COMPONENT_OPERATOR(-=)
ALIA_COMPONENT_OPERATOR(*=)
ALIA_COMPONENT_OPERATOR(/=)
#undef ALIA_COMPONENT_OPERATOR

// scalar arithmetic assignment operators
#define ALIA_SCALAR_OPERATOR(op) \
    template<unsigned N, class T, class Scalar> \
    vector<N,T>& operator op(vector<N,T>& a, Scalar b) \
    { \
        for (unsigned i = 0; i < N; ++i) \
            a[i] op b; \
        return a; \
    }
ALIA_SCALAR_OPERATOR(*=)
ALIA_SCALAR_OPERATOR(/=)
ALIA_SCALAR_OPERATOR(%=)
#undef ALIA_SCALAR_OPERATOR

// componentwise operators
#define ALIA_COMPONENT_OPERATOR(op) \
    template<unsigned N, class T> \
    vector<N,T> operator op(vector<N,T> const& a, vector<N,T> const& b) \
    { \
        vector<N,T> r; \
        for (unsigned i = 0; i < N; ++i) \
            r[i] = a[i] op b[i]; \
        return r; \
    }
ALIA_COMPONENT_OPERATOR(+)
ALIA_COMPONENT_OPERATOR(-)
ALIA_COMPONENT_OPERATOR(*)
ALIA_COMPONENT_OPERATOR(/)
#undef ALIA_COMPONENT_OPERATOR

// scalar operators
#define ALIA_COMMUTATIVE_SCALAR_OPERATOR(op) \
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
#define ALIA_SCALAR_OPERATOR(op) \
    template<unsigned N, class T, class Scalar> \
    vector<N,T> operator op(vector<N,T> const& a, Scalar b) \
    { \
        vector<N,T> r; \
        for (unsigned i = 0; i < N; ++i) \
            r[i] = a[i] op b; \
        return r; \
    }
ALIA_COMMUTATIVE_SCALAR_OPERATOR(*)
ALIA_SCALAR_OPERATOR(/)
ALIA_SCALAR_OPERATOR(%)
#undef ALIA_COMMUTATIVE_SCALAR_OPERATOR
#undef ALIA_SCALAR_OPERATOR

// unary negation
template<unsigned N, class T>
vector<N,T> operator-(vector<N,T> const& v)
{
    vector<N,T> r;
    for (unsigned i = 0; i < N; ++i)
        r[i] = -v[i];
    return r;
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

// Create a vector whose components all have the same value.
template<unsigned N, class T>
vector<N,T> uniform_vector(T value)
{
    vector<N,T> r;
    for (unsigned i = 0; i < N; ++i)
        r[i] = value;
    return r;
}

// hash function
} namespace std {
    template<unsigned N, class T>
    struct hash<alia::vector<N,T> >
    {
        size_t operator()(alia::vector<N,T> const& v) const
        {
            size_t h = 0;
            for (unsigned i = 0; i != N; ++i)
                h = alia::combine_hashes(h, hash<T>()(v[i]));
            return h;
        }
    };
} namespace alia {

// BOX

// A box is an N-dimensional generalization of a rectangle. In one dimension,
// it's a range; in two, a rectangle; in three, a rectangular prism; etc.
// It's always axis-aligned. It's represented by a corner point (the one with
// the smaller coordinates) and its size.
template<unsigned N, class T>
struct box
{
    box() {}

    box(vector<N,T> const& corner, vector<N,T> const& size)
      : corner(corner), size(size)
    {}

    // explicit conversion from a box with a different coordinate type
    template<class OtherT>
    explicit box(box<N,OtherT> const& other)
      : corner(other.corner), size(other.size)
    {}

    vector<N,T> corner, size;
};

template<unsigned N, typename T>
box<N,T> make_box(vector<N,T> const& corner, vector<N,T> const& size)
{ return box<N,T>(corner, size); }

template<unsigned N, typename T>
std::ostream& operator<<(std::ostream& out, box<N,T> const& box)
{
    out << "[corner: " << box.corner << ", size: " << box.size << "]";
    return out;
}

// equality operators
template<unsigned N, class T>
bool operator==(box<N,T> const& a, box<N,T> const& b)
{ return a.corner == b.corner && a.size == b.size; }
template<unsigned N, class T>
bool operator!=(box<N,T> const& a, box<N,T> const& b)
{ return !(a == b); }
// < operator
template<unsigned N, class T>
bool operator<(box<N,T> const& a, box<N,T> const& b)
{
    return a.corner < b.corner ||
        a.corner == b.corner && a.size < b.size;
}

template<unsigned N, class T>
vector<N,T> get_center(box<N,T> const& b) { return b.corner + b.size / 2; }

template<unsigned N, class T>
vector<N,T> get_low_corner(box<N,T> const& b) { return b.corner; }

template<unsigned N, class T>
vector<N,T> get_high_corner(box<N,T> const& b) { return b.corner + b.size; }

// Is the point p inside the given box?
template<unsigned N, class T>
bool is_inside(box<N,T> const& box, vector<N,T> const& p)
{
    for (unsigned i = 0; i < N; ++i)
    {
        if (p[i] < box.corner[i] || p[i] >= box.corner[i] + box.size[i])
            return false;
    }
    return true;
}

// Does the projection of p along the given axis lie within the projection of
// the box along the same axis?
template<unsigned N, class T>
bool is_inside(box<N,T> const& box, unsigned axis, T p)
{
    assert(axis < N);
    return p >= box.corner[axis] &&
        p < box.corner[axis] + box.size[axis];
}

// Compute the intersection of the two boxes.
// The return value is whether or not the boxes actually intersect.
// The result is only valid if they intersect.
template<unsigned N, class T>
bool compute_intersection(box<N,T>* result, box<N,T> const& box1,
    box<N,T> const& box2)
{
    for (unsigned i = 0; i < N; i++)
    {
        T low = box1.corner[i] > box2.corner[i] ?
            box1.corner[i] : box2.corner[i];
        T high1 = box1.corner[i] + box1.size[i];
        T high2 = box2.corner[i] + box2.size[i];
        T high = high1 < high2 ? high1 : high2;
        if (low >= high)
            return false;
        result->corner[i] = low;
        result->size[i] = high - low;
    }
    return true;
}

// Test if the two boxes are overlapping.
template<unsigned N, class T>
bool overlapping(box<N,T> const& box1, box<N,T> const& box2)
{
    for (unsigned i = 0; i < N; i++)
    {
        T low = (std::max)(box1.corner[i], box2.corner[i]);
        T high = (std::min)(get_high_corner(box1)[i],
            get_high_corner(box2)[i]);
        if (low > high)
            return false;
    }
    return true;
}

// Add a uniform border around the given box and return the expanded box.
template<unsigned N, class T>
box<N,T> add_border(box<N,T> const& box, T border)
{
    return alia::box<N,T>(
        box.corner - uniform_vector<N,T>(border),
        box.size + uniform_vector<N,T>(border * 2));
}

// Add a border around the given box and return the expanded box.
template<unsigned N, class T>
box<N,T> add_border(box<N,T> const& box, vector<N,T> const& border)
{
    return alia::box<N,T>(box.corner - border, box.size + border * 2);
}

template<typename T>
void make_polygon(vector<2,T>* vertices, box<2,T> const& box)
{
    vertices[0] = box.corner;
    vertices[1] = box.corner + make_vector<T>(box.size[0], 0);
    vertices[2] = box.corner + box.size;
    vertices[3] = box.corner + make_vector<T>(0, box.size[1]);
}

// Expand a box (if necessary) to include the given point. - If an expansion is necessary,
// it's done so that the point lies on the edge of the box.
template<unsigned N, class T>
void expand_box_to_include_point(box<N,T>& box, vector<N,T> const& point)
{
    for (unsigned i = 0; i != N; ++i)
    {
        // Expand in the negative direction.
        if (box.corner[i] > point[i])
        {
            box.size[i] += box.corner[i] - point[i];
            box.corner[i] = point[i];
        }
        // Expand in the positive direction.
        if (box.corner[i] + box.size[i] < point[i])
        {
            box.size[i] = point[i] - box.corner[i];
        }
    }
}

template<typename T>
int get_edge_index(box<2,T> const& box, vector<2,T> const& point)
{
    double tol = 1.0e-10;
    int index = -1;
    if((std::fabs(point[1] - box.corner[1]) <= tol) && (point[0] >= box.corner[0]))
    {
        index = 0;
    }
    if((std::fabs(point[0] - (box.corner[0] + box.size[0])) <= tol) && (point[1] >= box.corner[1]))
    {
        index = 1;
    }
    if((std::fabs(point[1] - (box.corner[1] + box.size[1])) <= tol)  && (point[0] > box.corner[0]))
    {
        index = 2;
    }
    if((std::fabs(point[0] - box.corner[0]) <= tol) && (point[1] > box.corner[1]))
    {
        index = 3;
    }
    return index;
}

// MATRIX

// M x N matrix class - T is the type of the scalar elements, M is the number
// of rows, and N is the number of columns.
template<unsigned M, unsigned N, class T>
class matrix
{
 public:
    matrix() {}

    // construct from a matrix of the same size but a different element type
    template<class OtherT>
    explicit matrix(matrix<M,N,OtherT> const& other)
    {
        typename matrix<M,N,OtherT>::const_iterator j = other.begin();
        iterator end = this->end();
        for (iterator i = begin(); i != end; ++i, ++j)
            *i = static_cast<T>(*j);
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

template<unsigned M, unsigned N, typename T>
std::ostream& operator<<(std::ostream& out, matrix<M,N,T> const& m)
{
    out << "[";
    for (unsigned i = 0; i != M; ++i)
    {
        for (unsigned j = 0; j != N; ++j)
        {
            if (i != 0 || j != 0)
                out << ", ";
            out << m(i, j);
        }
    }
    out << "]";
    return out;
}

// 2x2 constructor
template<class T>
matrix<2,2,T> make_matrix(
    T v00, T v01,
    T v10, T v11)
{
    matrix<2,2,T> m;
    m(0, 0) = v00;
    m(0, 1) = v01;
    m(1, 0) = v10;
    m(1, 1) = v11;
    return m;
}
// 3x3 constructor
template<class T>
matrix<3,3,T> make_matrix(
    T v00, T v01, T v02,
    T v10, T v11, T v12,
    T v20, T v21, T v22)
{
    matrix<3,3,T> m;
    m(0, 0) = v00;
    m(0, 1) = v01;
    m(0, 2) = v02;
    m(1, 0) = v10;
    m(1, 1) = v11;
    m(1, 2) = v12;
    m(2, 0) = v20;
    m(2, 1) = v21;
    m(2, 2) = v22;
    return m;
}
// 4x4 constructor
template<class T>
matrix<4,4,T> make_matrix(
    T v00, T v01, T v02, T v03,
    T v10, T v11, T v12, T v13,
    T v20, T v21, T v22, T v23,
    T v30, T v31, T v32, T v33)
{
    matrix<4,4,T> m;
    m(0, 0) = v00;
    m(0, 1) = v01;
    m(0, 2) = v02;
    m(0, 3) = v03;
    m(1, 0) = v10;
    m(1, 1) = v11;
    m(1, 2) = v12;
    m(1, 3) = v13;
    m(2, 0) = v20;
    m(2, 1) = v21;
    m(2, 2) = v22;
    m(2, 3) = v23;
    m(3, 0) = v30;
    m(3, 1) = v31;
    m(3, 2) = v32;
    m(3, 3) = v33;
    return m;
}

template<unsigned M, unsigned N, class T, class Scalar>
matrix<M,N,T> operator*(Scalar s, matrix<M,N,T> const& m)
{
    matrix<M,N,T> r = m;
    r *= s;
    return r;
}

// matrix multiplication
template <class T, unsigned M, unsigned N, unsigned O>
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

// Get the identity matrix.
template<unsigned N, class T>
matrix<N,N,T> identity_matrix()
{
    matrix<N,N,T> m;
    typename matrix<N,N,T>::iterator p = m.begin();
    for (unsigned i = 0; i < N; ++i)
        for (unsigned j = 0; j < N; ++j, ++p)
            *p = static_cast<T>((i == j) ? 1 : 0);
    return m;
}

// Get the inverse of the given matrix.
// 3x3 case
template<class T>
matrix<3,3,T> inverse(matrix<3,3,T> const& m)
{
    matrix<3,3,T> inv =
        make_matrix<T>(
            m(1,1) * m(2,2) - m(1,2) * m(2,1),
            m(0,2) * m(2,1) - m(0,1) * m(2,2),
            m(0,1) * m(1,2) - m(0,2) * m(1,1),
            m(1,2) * m(2,0) - m(1,0) * m(2,2),
            m(0,0) * m(2,2) - m(0,2) * m(2,0),
            m(0,2) * m(1,0) - m(0,0) * m(1,2),
            m(1,0) * m(2,1) - m(1,1) * m(2,0),
            m(0,1) * m(2,0) - m(0,0) * m(2,1),
            m(0,0) * m(1,1) - m(0,1) * m(1,0));
    T det = m(0,0) * inv(0,0) + m(0,1) * inv(1,0) + m(0,2) * inv(2,0);
    if (det == 0)
        return make_matrix<T>(0, 0, 0, 0, 0, 0, 0, 0, 0);
    inv *= T(1) / det;
    return inv;
}
// 2x2 case
template<class T>
matrix<2,2,T> inverse(matrix<2,2,T> const& m)
{
    T det = m(0,0) * m(1,1) - m(0,1) * m(1,0);
    if (det == 0)
        return make_matrix<T>(0, 0, 0, 0);
    T inv_det = static_cast<T>(1.0) / det;
    matrix<2,2,T> inv = make_matrix(m(1,1), -m(0,1), -m(1,0), m(0,0));
    inv *= inv_det;
    return inv;
}

// hash function
} namespace std {
    template<unsigned M, unsigned N, class T>
    struct hash<alia::matrix<M,N,T> >
    {
        size_t operator()(alia::matrix<M,N,T> const& m) const
        {
            size_t h = 0;
            for (unsigned i = 0; i != M; ++i)
            {
                for (unsigned j = 0; j != N; ++j)
                    h = alia::combine_hashes(h, hash<T>()(m(i, j)));
            }
            return h;
        }
    };
} namespace alia {

// ANGLES

static const double pi = 3.1415926535897932384626433832795;

static inline double degrees_to_radians(double degrees)
{ return degrees * (pi / 180); }

static inline double radians_to_degrees(double radians)
{ return radians * (180 / pi); }

// 2D TRANSFORMATIONS

// Generate a rotation matrix.
template<class T>
matrix<3,3,T> rotation_matrix(T a)
{
    T c = std::cos(a), s = std::sin(a);
    return make_matrix<T>(
        c, -s, 0,
        s,  c, 0,
        0,  0, 1);
}

// Generate a translation matrix.
template<class T>
matrix<3,3,T> translation_matrix(vector<2,T> const& v)
{
    return make_matrix<T>(
        1, 0, v[0],
        0, 1, v[1],
        0, 0,    1);
}

// Generate a scaling matrix.
template<class T>
matrix<3,3,T> scaling_matrix(vector<2,T> const& v)
{
    return make_matrix<T>(
        v[0],    0, 0,
           0, v[1], 0,
           0,    0, 1);
}

// Transform a vector by a transformation matrix.
template<class T>
vector<2,T> transform(matrix<3,3,T> const& m, vector<2,T> const& v)
{
    return make_vector(
        v[0] * m(0,0) + v[1] * m(0,1) + m(0,2),
        v[0] * m(1,0) + v[1] * m(1,1) + m(1,2));
}

// Transform a box by a transformation matrix.
template<typename T>
box<2,T>
transform_box(matrix<3,3,T> const& m, box<2,T> const& b)
{
    // Start with a box that just includes the transformation of one corner of the
    // original box.
    box<2,T> result = make_box(transform(m, b.corner), make_vector<T>(0, 0));
    // Now expand that box to include the transformation of each of the other corners.
    expand_box_to_include_point(result,
        transform(m, b.corner + make_vector<T>(b.size[0], 0)));
    expand_box_to_include_point(result,
        transform(m, b.corner + b.size));
    expand_box_to_include_point(result,
        transform(m, b.corner + make_vector<T>(0, b.size[1])));
    return result;
}

// CUBIC BEZIERS

// unit_cubic_bezier represents a cubic bezier whose end points are (0, 0)
// and (1, 1).
struct unit_cubic_bezier
{
    vector<2,double> p1, p2;

    unit_cubic_bezier() {}
    unit_cubic_bezier(vector<2,double> const& p1, vector<2,double> const& p2)
      : p1(p1), p2(p2)
    {}
    unit_cubic_bezier(double p1x, double p1y, double p2x, double p2y)
    { p1[0] = p1x; p1[1] = p1y; p2[0] = p2x; p2[1] = p2y; }
};

// Evaluate a unit_cubic_bezier at the given x value.
// Since this is an approximation, the caller must specify an epsilon value
// to control the error in the result.
double
eval_curve_at_x(unit_cubic_bezier const& curve, double x, double epsilon);

}

#endif
