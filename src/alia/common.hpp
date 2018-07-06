#ifndef ALIA_COMMON_HPP
#define ALIA_COMMON_HPP

#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>

// This file defines some generic functionality that's commonly used throughout
// alia.

namespace alia {

typedef long long counter_type;

using std::int16_t;
using std::int8_t;
using std::uint16_t;
using std::uint8_t;

typedef std::string string;

typedef char const* utf8_ptr;

struct utf8_string
{
    utf8_ptr begin, end;

    utf8_string()
    {
    }
    utf8_string(utf8_ptr begin, utf8_ptr end) : begin(begin), end(end)
    {
    }
};

static inline bool
is_empty(utf8_string const& text)
{
    return text.begin == text.end;
}

static inline utf8_string
as_utf8_string(string const& text)
{
    return utf8_string(text.c_str(), text.c_str() + text.length());
}

template<typename T>
T
clamp(T x, T min, T max)
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
struct null_flag_set
{
};
static null_flag_set const NO_FLAGS = null_flag_set();

template<class Tag>
struct flag_set
{
    unsigned code;
    flag_set()
    {
    }
    flag_set(null_flag_set) : code(0)
    {
    }
    explicit flag_set(unsigned code) : code(code)
    {
    }
    // allows use within if statements without other unintended conversions
    typedef unsigned flag_set::*unspecified_bool_type;
    operator unspecified_bool_type() const
    {
        return code != 0 ? &flag_set::code : 0;
    }
};

template<class Tag>
flag_set<Tag>
operator|(flag_set<Tag> a, flag_set<Tag> b)
{
    return flag_set<Tag>(a.code | b.code);
}
template<class Tag>
flag_set<Tag>&
operator|=(flag_set<Tag>& a, flag_set<Tag> b)
{
    a.code |= b.code;
    return a;
}
template<class Tag>
flag_set<Tag> operator&(flag_set<Tag> a, flag_set<Tag> b)
{
    return flag_set<Tag>(a.code & b.code);
}
template<class Tag>
flag_set<Tag>&
operator&=(flag_set<Tag>& a, flag_set<Tag> b)
{
    a.code &= b.code;
    return a;
}
template<class Tag>
bool
operator==(flag_set<Tag> a, flag_set<Tag> b)
{
    return a.code == b.code;
}
template<class Tag>
bool
operator!=(flag_set<Tag> a, flag_set<Tag> b)
{
    return a.code != b.code;
}
template<class Tag>
bool
operator<(flag_set<Tag> a, flag_set<Tag> b)
{
    return a.code < b.code;
}
template<class Tag>
flag_set<Tag>
operator~(flag_set<Tag> a)
{
    return flag_set<Tag>(~a.code);
}
} // namespace alia

namespace std {
template<class Tag>
struct hash<alia::flag_set<Tag>>
{
    size_t
    operator()(alia::flag_set<Tag> const& set) const
    {
        return hash<unsigned>()(set.code);
    }
};
} // namespace std

namespace alia {
#define ALIA_DEFINE_FLAG_TYPE(type_prefix)                                     \
    struct type_prefix##_flag_tag                                              \
    {                                                                          \
    };                                                                         \
    typedef alia::flag_set<type_prefix##_flag_tag> type_prefix##_flag_set;

#define ALIA_DEFINE_FLAG(type_prefix, code, name)                              \
    static unsigned const name##_CODE = code;                                  \
    static alia::flag_set<type_prefix##_flag_tag> const name(code);

// Inspired by Boost, inheriting from noncopyable disables copying for a type.
// The namespace prevents unintended ADL if used by applications.
namespace impl {
namespace noncopyable_ {
struct noncopyable
{
    noncopyable()
    {
    }

 private:
    noncopyable(noncopyable const& other);
    noncopyable&
    operator=(noncopyable const& other);
};
} // namespace noncopyable_
} // namespace impl
typedef impl::noncopyable_::noncopyable noncopyable;

struct exception : std::exception
{
    exception(string const& msg) : msg_(new string(msg))
    {
        std::cout << msg << std::endl;
    }

    ~exception() throw()
    {
    }

    virtual char const*
    what() const throw()
    {
        return msg_->c_str();
    }

    // Add another level of context to the error messsage.
    void
    add_context(string const& str)
    {
        *msg_ += "\n" + str;
    }

 private:
    std::shared_ptr<string> msg_;
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

    vector()
    {
    }

    // explicit conversion from vectors with other element types
    template<class OtherT>
    explicit vector(vector<N, OtherT> const& other)
    {
        for (unsigned i = 0; i < N; ++i)
            (*this)[i] = static_cast<T>(other[i]);
    }

 private:
    T elements[N];
};
// 2D constructor
template<class T>
vector<2, T>
make_vector(T x, T y)
{
    vector<2, T> v;
    v[0] = x;
    v[1] = y;
    return v;
}
// 3D constructor
template<class T>
vector<3, T>
make_vector(T x, T y, T z)
{
    vector<3, T> v;
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
    static bool
    apply(vector<N, T> const& a, vector<N, T> const& b)
    {
        return a[I - 1] == b[I - 1]
               && vector_equality_test<N, T, I - 1>::apply(a, b);
    }
};
template<unsigned N, class T>
struct vector_equality_test<N, T, 0>
{
    static bool
    apply(vector<N, T> const& a, vector<N, T> const& b)
    {
        return true;
    }
};
} // namespace impl
template<unsigned N, class T>
bool
operator==(vector<N, T> const& a, vector<N, T> const& b)
{
    return impl::vector_equality_test<N, T, N>::apply(a, b);
}
template<unsigned N, class T>
bool
operator!=(vector<N, T> const& a, vector<N, T> const& b)
{
    return !(a == b);
}
// < operator
template<unsigned N, class T>
bool
operator<(vector<N, T> const& a, vector<N, T> const& b)
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
std::ostream&
operator<<(std::ostream& out, vector<N, T> const& v)
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
struct none_type
{
};
static none_type const none;
template<class T>
struct optional
{
    typedef T value_type;
    optional() : valid_(false)
    {
    }
    optional(T const& value) : value_(value), valid_(true)
    {
    }
    optional(none_type) : valid_(false)
    {
    }
    optional&
    operator=(T const& value)
    {
        value_ = value;
        valid_ = true;
        return *this;
    }
    optional& operator=(none_type)
    {
        valid_ = false;
        return *this;
    }
    // allows use within if statements without other unintended conversions
    typedef bool optional::*unspecified_bool_type;
    operator unspecified_bool_type() const
    {
        return valid_ ? &optional::valid_ : 0;
    }
    T const&
    get() const
    {
        assert(valid_);
        return value_;
    }
    T&
    get()
    {
        assert(valid_);
        return value_;
    }

 private:
    T value_;
    bool valid_;
};
template<class T>
T const&
get(optional<T> const& opt)
{
    return opt.get();
}
template<class T>
T&
get(optional<T>& opt)
{
    return opt.get();
}

template<class T>
bool
operator==(optional<T> const& a, optional<T> const& b)
{
    return a ? (b && get(a) == get(b)) : !b;
}
template<class T>
bool
operator!=(optional<T> const& a, optional<T> const& b)
{
    return !(a == b);
}
template<class T>
bool
operator<(optional<T> const& a, optional<T> const& b)
{
    return b && (a ? get(a) < get(b) : true);
}

template<class T>
optional<T>
some(T const& x)
{
    return optional<T>(x);
}

template<class Container>
struct raii_adaptor : Container
{
    ~raii_adaptor()
    {
        Container::end();
    }
};

// any is effectively an implementation of std/boost::any that a) can be used
// without a dependence on C++17/Boost and b) provides unsafe casts.
struct any
{
    // default constructor
    any() : holder_(0)
    {
    }
    // destructor
    ~any()
    {
        delete holder_;
    }
    // copy constructor
    any(any const& other) : holder_(other.holder_ ? other.holder_->clone() : 0)
    {
    }
    // move constructor
    any(any&& other) : holder_(other.holder_)
    {
        other.holder_ = 0;
    }
    // constructor for concrete values
    template<class T>
    explicit any(T const& value) : holder_(new typed_value_holder<T>(value))
    {
    }
    // constructor for concrete values (by rvalue)
    template<typename T>
    any(T&& value,
        std::enable_if_t<!std::is_same<any&, T>::value>* = 0,
        std::enable_if_t<!std::is_const<T>::value>* = 0)
        : holder_(new typed_value_holder<typename std::decay<T>::type>(
              static_cast<T&&>(value)))
    {
    }
    // copy assignment operator
    any&
    operator=(any const& other)
    {
        delete holder_;
        holder_ = other.holder_ ? other.holder_->clone() : 0;
        return *this;
    }
    // move assignment operator
    any&
    operator=(any&& other)
    {
        delete holder_;
        holder_ = other.holder_;
        other.holder_ = 0;
        return *this;
    }
    // swap
    void
    swap(any& other)
    {
        std::swap(holder_, other.holder_);
    }
    // assignment operator for concrete values
    template<class T>
    any&
    operator=(T const& value)
    {
        delete holder_;
        holder_ = new typed_value_holder<T>(value);
        return *this;
    }
    // assignment operator for concrete values (by rvalue)
    template<class T>
    any&
    operator=(T&& value)
    {
        any(static_cast<T&&>(value)).swap(*this);
        return *this;
    }
    // value holder
    struct untyped_value_holder
    {
        virtual ~untyped_value_holder()
        {
        }
        virtual untyped_value_holder*
        clone() const = 0;
    };
    template<class T>
    struct typed_value_holder : untyped_value_holder
    {
        explicit typed_value_holder(T const& value) : value(value)
        {
        }
        explicit typed_value_holder(T&& value) : value(static_cast<T&&>(value))
        {
        }
        untyped_value_holder*
        clone() const
        {
            return new typed_value_holder(value);
        }
        T value;
    };
    untyped_value_holder* holder_;
};
static inline void
swap(any& a, any& b)
{
    a.swap(b);
}
template<class T>
T const*
any_cast(any const& a)
{
    any::typed_value_holder<T> const* ptr
        = dynamic_cast<any::typed_value_holder<T> const*>(a.holder_);
    return ptr ? &ptr->value : 0;
}
template<class T>
T*
any_cast(any& a)
{
    any::typed_value_holder<T>* ptr
        = dynamic_cast<any::typed_value_holder<T>*>(a.holder_);
    return ptr ? &ptr->value : 0;
}
template<class T>
T const&
unsafe_any_cast(any const& a)
{
    assert(dynamic_cast<any::typed_value_holder<T> const*>(a.holder_));
    return static_cast<any::typed_value_holder<T> const*>(a.holder_)->value;
}
template<class T>
T&
unsafe_any_cast(any& a)
{
    assert(dynamic_cast<any::typed_value_holder<T>*>(a.holder_));
    return static_cast<any::typed_value_holder<T>*>(a.holder_)->value;
}

struct id_interface;

// Invoke the standard hash function for a value.
template<class T>
size_t
invoke_hash(T const& x)
{
    return std::hash<T>()(x);
}

// Combine two hash values.
size_t static inline combine_hashes(size_t a, size_t b)
{
    return a ^ (0x9e3779b9 + (a << 6) + (a >> 2) + b);
}

// ALIA_UNUSED is used to denote that a variable may be unused.
#ifdef __GNUC__
#define ALIA_UNUSED [[gnu::unused]]
#else
#define ALIA_UNUSED
#endif
} // namespace alia

#endif
