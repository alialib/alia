#ifndef ALIA_BOX_HPP
#define ALIA_BOX_HPP

#include <cassert>
#include <alia/forward.hpp>
#include <alia/point.hpp>
#include <alia/vector.hpp>
#include <algorithm>

namespace alia {

// A box is an n-dimensional generalization of a rectangle.  In one dimension,
// it's a range; in two, a rectangle; in three, a rectangular prism; etc.  It
// is always axis-aligned.  It's represented by a corner point (the one with
// the smaller coordinates) and a vector representing its size.
template<unsigned N, class T>
struct box
{
    box() {}

    box(point<N,T> const& corner, vector<N,T> const& size)
      : corner(corner), size(size)
    {}

    // conversion from a box with a different coordinate type
    template<typename OtherT>
    explicit box(box<N,OtherT> const& other)
      : corner(other.corner), size(other.size)
    {}

    point<N,T> corner;
    vector<N,T> size;
};

template<unsigned N, class T>
bool operator==(box<N,T> const& a, box<N,T> const& b)
{ return a.corner == b.corner && a.size == b.size; }

template<unsigned N, class T>
bool operator!=(box<N,T> const& a, box<N,T> const& b)
{ return !(a == b); }

template<unsigned N, class T>
point<N,T> get_center(box<N,T> const& b) { return b.corner + b.size / 2; }

template<unsigned N, class T>
point<N,T> get_low_corner(box<N,T> const& b) { return b.corner; }

template<unsigned N, class T>
point<N,T> get_high_corner(box<N,T> const& b) { return b.corner + b.size; }

// Is the point p inside the given box?
template<unsigned N, typename T>
bool is_inside(box<N,T> const& box, point<N,T> const& p)
{
    for (unsigned i = 0; i < N; ++i)
    {
        if (p[i] < box.corner[i] || p[i] >= box.corner[i] + box.size[i])
            return false;
    }
    return true;
}

// is_inside() test along only one axis
template<unsigned N, typename T>
bool is_inside(box<N,T> const& box, unsigned axis, T p)
{
    assert(axis < N);
    return p >= box.corner[axis] &&
        p < box.corner[axis] + box.size[axis];
}

// Compute the intersection of the two boxes.
// The return value is whether or not the boxes actually intersect.
// The result is only valid if they intersect.
template<unsigned N, typename T>
bool compute_intersection(box<N,T>* result, box<N,T> const& box1,
    box<N,T> const& box2)
{
    for (unsigned i = 0; i < N; i++)
    {
        T low = (std::max)(box1.corner[i], box2.corner[i]);
        T high = (std::min)(get_high_corner(box1)[i],
            get_high_corner(box2)[i]);
        if (low >= high)
            return false;
        result->corner[i] = low;
        result->size[i] = high - low;
    }
    return true;
}

// Test if the two boxes are overlapping.
template<unsigned N, typename T>
bool overlapping(box<N,T> const& box1, box<N,T> const& box2)
{
    for (unsigned i = 0; i < N; i++)
    {
        T low = (std::max)(box1.corner[i], box2.corner[i]);
        T high = (std::min)(get_high_corner(box1)[i],
            get_high_corner(box2)[i]);
        if (low >= high)
            return false;
    }
    return true;
}

// Add a uniform border around the given box and return the expanded box.
template<unsigned N, typename T>
box<N,T> add_border(box<N,T> const& box, T border)
{
    return alia::box<N,T>(
        box.corner - uniform_vector<N,T>(border),
        box.size + uniform_vector<N,T>(border * 2));
}

// Add a border around the given box and return the expanded box.
template<unsigned N, typename T>
box<N,T> add_border(box<N,T> const& box, vector<N,T> const& border)
{
    return alia::box<N,T>(box.corner - border, box.size + border * 2);
}

template<typename T>
void make_polygon(point<2,T>* vertices, box<2,T> const& box)
{
    vertices[0] = box.corner;
    vertices[1] = box.corner + vector<2,T>(box.size[0], 0);
    vertices[2] = box.corner + box.size;
    vertices[3] = box.corner + vector<2,T>(0, box.size[1]);
}

}

#endif
