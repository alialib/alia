#ifndef ALIA_COMMON_HPP
#define ALIA_COMMON_HPP

#include <exception>
#include <string>
#include <memory>
#include <cassert>
#include <ostream>

// This file defines some generic functionality that's commonly used throughout
// alia.

namespace alia {

typedef long long counter_type;

typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;

typedef std::string string;

typedef char const* utf8_ptr;

struct utf8_string
{
    utf8_ptr begin, end;

    utf8_string() {}
    utf8_string(utf8_ptr begin, utf8_ptr end)
      : begin(begin), end(end)
    {}
};

static inline bool is_empty(utf8_string const& text)
{ return text.begin == text.end; }

static inline utf8_string as_utf8_string(string const& text)
{ return utf8_string(text.c_str(), text.c_str() + text.length()); }

// Figure out which shared_ptr to use.
#ifdef _MSC_VER
    #if (_MSC_VER >= 1600)
        #define alia__shared_ptr std::shared_ptr
    #else
        #define alia__shared_ptr std::tr1::shared_ptr
    #endif
#else
    #define alia__shared_ptr std::shared_ptr
#endif

template<typename T>
T clamp(T x, T min, T max)
{
    assert(min <= max);
    return (std::min)((std::max)(x, min), max);
}

// A flag_set is a set of flags, each of which represents a boolean property.
// It is implemented as a simple unsigned integer, where each bit represents
// a different property.
// (The property codes must be defined manually as constants.)
// The advantage of using this over plain unsigned integers is that this is
// type-safe and slightly more explicit.
// A flag_set has a Tag type that identifies the set of properties that go with
// it. Only properties/sets with the same tag can be combined.

// NO_FLAGS can be implicitly converted to any type of flag_set.
struct null_flag_set {};
static null_flag_set const NO_FLAGS = null_flag_set();

template<class Tag>
struct flag_set
{
    unsigned code;
    flag_set() {}
    flag_set(null_flag_set) : code(0) {}
    explicit flag_set(unsigned code) : code(code) {}
    // allows use within if statements without other unintended conversions
    typedef unsigned flag_set::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return code != 0 ? &flag_set::code : 0; }
};

template<class Tag>
flag_set<Tag> operator|(flag_set<Tag> a, flag_set<Tag> b)
{ return flag_set<Tag>(a.code | b.code); }
template<class Tag>
flag_set<Tag>& operator|=(flag_set<Tag>& a, flag_set<Tag> b)
{ a.code |= b.code; return a; }
template<class Tag>
flag_set<Tag> operator&(flag_set<Tag> a, flag_set<Tag> b)
{ return flag_set<Tag>(a.code & b.code); }
template<class Tag>
flag_set<Tag>& operator&=(flag_set<Tag>& a, flag_set<Tag> b)
{ a.code &= b.code; return a; }
template<class Tag>
bool operator==(flag_set<Tag> a, flag_set<Tag> b)
{ return a.code == b.code; }
template<class Tag>
bool operator!=(flag_set<Tag> a, flag_set<Tag> b)
{ return a.code != b.code; }
template<class Tag>
bool operator<(flag_set<Tag> a, flag_set<Tag> b)
{ return a.code < b.code; }

// A macro for defining flag constants.
#define ALIA_DEFINE_FLAG_CODE(Tag, code, name) \
    static unsigned const name##_CODE = code; \
    static alia::flag_set<Tag> const name(code);

// Inspired by Boost, inheriting from noncopyable disables copying for a type.
namespace noncopyable_ // prevents unintended ADL if used by applications
{
    struct noncopyable
    {
        noncopyable() {}
     private:
        noncopyable(noncopyable const& other);
        noncopyable& operator=(noncopyable const& other);
    };
}
typedef noncopyable_::noncopyable noncopyable;

struct exception : std::exception
{
    exception(string const& msg)
      : msg_(new string(msg))
    {}
    ~exception() throw() {}

    virtual char const* what() const throw()
    { return msg_->c_str(); }

    // Add another level of context to the error messsage.
    void add_context(string const& str)
    { *msg_ += "\n" + str; }

 private:
    alia__shared_ptr<string> msg_;
};

// vector<N,T> represents an N-dimensional geometric vector with elements of
// type T.
template<unsigned N, class T>
struct vector
{
    // allow external access to template parameters
    typedef T value_type;
    static const unsigned dimensionality = N;

    // element accessors
    T operator[](unsigned i) const
    {
        assert(i < N);
        return elements[i];
    }
    T& operator[](unsigned i)
    {
        assert(i < N);
        return elements[i];
    }

    vector() {}

    // explicit conversion from vectors with other element types
    template<class OtherT>
    explicit vector(vector<N,OtherT> const& other)
    {
        for (unsigned i = 0; i < N; ++i)
            (*this)[i] = static_cast<T>(other[i]);
    }

 private:
    T elements[N];
};
// 2D constructor
template<class T>
vector<2,T> make_vector(T x, T y)
{
    vector<2,T> v;
    v[0] = x;
    v[1] = y;
    return v;
}
// 3D constructor
template<class T>
vector<3,T> make_vector(T x, T y, T z)
{
    vector<3,T> v;
    v[0] = x;
    v[1] = y;
    v[2] = z;
    return v;
}
// equality operators
namespace impl {
    template<unsigned N, class T, unsigned I>
    struct vector_equality_test
    {
        static bool apply(vector<N,T> const& a, vector<N,T> const& b)
        {
            return a[I - 1] == b[I - 1] &&
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
// < operator
template<unsigned N, class T>
bool operator<(vector<N,T> const& a, vector<N,T> const& b)
{
    for (unsigned i = 0; i < N; ++i)
    {
        if (a[i] < b[i])
            return true;
        if (b[i] < a[i])
            return false;
    }
    return false;
}
// streaming
template<unsigned N, class T>
std::ostream& operator<<(std::ostream& out, vector<N,T> const& v)
{
    out << "(";
    for (unsigned i = 0; i != N; ++i)
    {
        if (i != 0)
            out << ", ";
        out << v[i];
    }
    out << ")";
    return out;
}

// optional<T> stores an optional value of type T (or no value).
struct none_type {};
static none_type none;
template<class T>
struct optional
{
    optional() : valid_(false) {}
    optional(T const& value) : value_(value), valid_(true) {}
    optional(none_type) : valid_(false) {}
    optional& operator=(T const& value)
    { value_ = value; valid_ = true; return *this; }
    optional& operator=(none_type)
    { valid_ = false; return *this; }
    // allows use within if statements without other unintended conversions
    typedef bool optional::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return valid_ ? &optional::valid_ : 0; }
    T const& get() const
    { assert(valid_); return value_; }
    T& get()
    { assert(valid_); return value_; }
 private:
    T value_;
    bool valid_;
};
template<class T>
T const& get(optional<T> const& opt)
{ return opt.get(); }
template<class T>
T& get(optional<T>& opt)
{ return opt.get(); }

template<class T>
bool operator==(optional<T> const& a, optional<T> const& b)
{
    return a ? (b && get(a) == get(b)) : !b;
}
template<class T>
bool operator!=(optional<T> const& a, optional<T> const& b)
{
    return !(a == b);
}
template<class T>
bool operator<(optional<T> const& a, optional<T> const& b)
{
    return b && (a ? get(a) < get(b) : true);
}

}

#endif
