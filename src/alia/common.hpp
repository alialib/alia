#ifndef ALIA_COMMON_HPP
#define ALIA_COMMON_HPP

#include <exception>
#include <string>
#include <memory>
#include <cassert>
#include <ostream>
#include <iostream>
#include <cstdint>

// This file defines some generic functionality that's commonly used throughout
// alia.

namespace alia {

typedef long long counter_type;

using std::int8_t;
using std::uint8_t;
using std::int16_t;
using std::uint16_t;

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
template<class Tag>
flag_set<Tag> operator~(flag_set<Tag> a)
{ return flag_set<Tag>(~a.code); }

}

namespace std
{
    template<class Tag>
    struct hash<alia::flag_set<Tag> >
    {
        size_t operator()(alia::flag_set<Tag> const& set) const
        {
            return hash<unsigned>()(set.code);
        }
    };
}

namespace alia {

#define ALIA_DEFINE_FLAG_TYPE(type_prefix) \
    struct type_prefix##_flag_tag {}; \
    typedef alia::flag_set<type_prefix##_flag_tag> type_prefix##_flag_set;

#define ALIA_DEFINE_FLAG(type_prefix, code, name) \
    static unsigned const name##_CODE = code; \
    static alia::flag_set<type_prefix##_flag_tag> const name(code);

// Inspired by Boost, inheriting from noncopyable disables copying for a type.
// The namespace prevents unintended ADL if used by applications.
namespace impl { namespace noncopyable_ {
struct noncopyable
{
    noncopyable() {}
 private:
    noncopyable(noncopyable const& other);
    noncopyable& operator=(noncopyable const& other);
};
}}
typedef impl::noncopyable_::noncopyable noncopyable;

struct exception : std::exception
{
    exception(string const& msg)
      : msg_(new string(msg))
    {
        std::cout << msg << std::endl;
    }

    ~exception() throw() {

    }

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
    typedef T value_type;
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

template<class T>
optional<T>
some(T const& x)
{ return optional<T>(x); }

template<class Container>
struct raii_adaptor : Container
{
    ~raii_adaptor() { Container::end(); }
};

struct id_interface;

// Accessors are the means by which UI elements access model state and values
// computed by the application for display in the UI.
// The goals of the accessor library are as follows.
// - Unify the interface to C-style data structures and OOP-style classes
//   (whose data members may be protected behind accessor functions).
// - Provide standard mechanisms for transforming the UI's view of model state
//   or applying constraints to its manipulations of that state.
// - Provide mechanisms for efficiently detecting changes in displayed values.
// - Ensure that the passing of values to the UI is as efficient and lazy as
//   possible.
//
// Accessors are passed by const reference into UI functions.
// They're typically created directly at the call site as function arguments
// and are only valid for the life of the function call.
// Accessor wrappers are templated and store copies of the actual wrapped
// accessor, which allows them to be easily composed at the call site, without
// requiring any memory allocation.
// UI functions are untemplated and lose the actual type of the accessor.
// One consequence of this is that a UI container cannot store its accessor
// for its entire scope and thus only has access to it within its begin
// function. If a container needs to set its accessor's value from within its
// scope, it can do so by reinvoking the UI context with a set_value_event
// that is processed by the container's begin function.
//
struct untyped_accessor_base
{
    // If this returns false, the underlying state has no value, so get()
    // should not be called.
    virtual bool is_gettable() const = 0;

    // An accessor must supply an ID which uniquely identifies its value.
    // The ID is required to be valid if is_gettable() returns true.
    // (It may be valid even if is_gettable() returns false, which would mean
    // that the accessor can identify its value but doesn't know it yet.)
    // The ID reference is only valid as long as the accessor itself is valid.
    virtual id_interface const& id() const = 0;

    // If is_settable() returns false, the accessor is currently read-only and
    // any UI controls associated with it should disallow user input.
    virtual bool is_settable() const = 0;
};
template<class T>
struct accessor : untyped_accessor_base
{
    typedef T value_type;

    // Get the value. The reference returned here is only guaranteed to be
    // valid as long as the accessor itself is valid.
    virtual T const& get() const = 0;

    // Set the value. (Only call if is_settable returns true.)
    virtual void set(T const& value) const = 0;
};

// Invoke the standard hash function for a value.
template<class T>
size_t invoke_hash(T const& x)
{ return std::hash<T>()(x); }

// Combine two hash values.
size_t static inline
combine_hashes(size_t a, size_t b)
{ return a ^ (0x9e3779b9 + (a << 6) + (a >> 2) + b); }

}

#endif
