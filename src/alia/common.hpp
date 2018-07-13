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
    noncopyable(noncopyable const& other) = delete;
    noncopyable&
    operator=(noncopyable const& other) = delete;
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

// optional<T> stores an optional value of type T (or no value).
struct none_type
{
    none_type() {}
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
