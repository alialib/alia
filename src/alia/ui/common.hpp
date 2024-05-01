#ifndef ALIA_UI_COMMON_HPP
#define ALIA_UI_COMMON_HPP

#include <alia/core/common.hpp>

namespace alia {

// Combine two hash values.
size_t inline combine_hashes(size_t a, size_t b)
{
    return a ^ (0x9e3779b9 + (a << 6) + (a >> 2) + b);
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
null_flag_set const NO_FLAGS = null_flag_set();

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
    explicit
    operator bool() const
    {
        return code != 0;
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
flag_set<Tag>
operator&(flag_set<Tag> a, flag_set<Tag> b)
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

template<class Tag>
struct std::hash<alia::flag_set<Tag>>
{
    size_t
    operator()(alia::flag_set<Tag> const& set) const
    {
        return hash<unsigned>()(set.code);
    }
};

#define ALIA_DEFINE_FLAG_TYPE(type_prefix)                                    \
    struct type_prefix##_flag_tag                                             \
    {                                                                         \
    };                                                                        \
    typedef alia::flag_set<type_prefix##_flag_tag> type_prefix##_flag_set;

#define ALIA_DEFINE_FLAG(type_prefix, code, name)                             \
    unsigned const name##_CODE = code;                                        \
    alia::flag_set<type_prefix##_flag_tag> const name(code);

#endif
