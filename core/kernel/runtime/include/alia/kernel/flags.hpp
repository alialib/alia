#pragma once

#include <cstdint>

namespace alia {

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
null_flag_set const NO_FLAGS = null_flag_set{};

template<class Tag, class Storage = std::uint32_t>
struct flag_set
{
    Storage code;
    flag_set()
    {
    }
    flag_set(null_flag_set) : code(0)
    {
    }
    explicit flag_set(Storage code) : code(code)
    {
    }
    explicit
    operator bool() const
    {
        return code != 0;
    }
};

template<class Tag, class Storage>
flag_set<Tag, Storage>
operator|(flag_set<Tag, Storage> a, flag_set<Tag, Storage> b)
{
    return flag_set<Tag, Storage>(a.code | b.code);
}
template<class Tag, class Storage>
flag_set<Tag, Storage>&
operator|=(flag_set<Tag, Storage>& a, flag_set<Tag, Storage> b)
{
    a.code |= b.code;
    return a;
}
template<class Tag, class Storage>
flag_set<Tag, Storage>
operator&(flag_set<Tag, Storage> a, flag_set<Tag, Storage> b)
{
    return flag_set<Tag, Storage>(a.code & b.code);
}
template<class Tag, class Storage>
flag_set<Tag, Storage>&
operator&=(flag_set<Tag, Storage>& a, flag_set<Tag, Storage> b)
{
    a.code &= b.code;
    return a;
}
template<class Tag, class Storage>
bool
operator==(flag_set<Tag, Storage> a, flag_set<Tag, Storage> b)
{
    return a.code == b.code;
}
template<class Tag, class Storage>
bool
operator!=(flag_set<Tag, Storage> a, flag_set<Tag, Storage> b)
{
    return a.code != b.code;
}
template<class Tag, class Storage>
bool
operator<(flag_set<Tag, Storage> a, flag_set<Tag, Storage> b)
{
    return a.code < b.code;
}
template<class Tag, class Storage>
flag_set<Tag, Storage>
operator~(flag_set<Tag, Storage> a)
{
    return flag_set<Tag, Storage>(~a.code);
}

template<class Tag, class Storage>
Storage
raw_code(flag_set<Tag, Storage> flags)
{
    return flags.code;
}

} // namespace alia

#define ALIA_DEFINE_FLAG_TYPE(Storage, type_prefix)                           \
    struct type_prefix##_flag_tag                                             \
    {                                                                         \
        using storage_type = Storage;                                         \
    };                                                                        \
    typedef alia::flag_set<type_prefix##_flag_tag, Storage>                   \
        type_prefix##_flag_set;

#define ALIA_DEFINE_FLAG(type_prefix, code, name)                             \
    typename type_prefix##_flag_tag::storage_type const name##_CODE = code;   \
    alia::flag_set<                                                           \
        type_prefix##_flag_tag,                                               \
        typename type_prefix##_flag_tag::storage_type> const name(code);
