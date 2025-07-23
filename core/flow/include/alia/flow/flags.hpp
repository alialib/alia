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
struct NullFlagSet
{
};
NullFlagSet const NO_FLAGS = NullFlagSet();

template<class Tag, class Storage = std::uint32_t>
struct FlagSet
{
    Storage code;
    FlagSet()
    {
    }
    FlagSet(NullFlagSet) : code(0)
    {
    }
    explicit FlagSet(Storage code) : code(code)
    {
    }
    explicit
    operator bool() const
    {
        return code != 0;
    }
};

template<class Tag, class Storage>
FlagSet<Tag, Storage>
operator|(FlagSet<Tag, Storage> a, FlagSet<Tag, Storage> b)
{
    return FlagSet<Tag, Storage>(a.code | b.code);
}
template<class Tag, class Storage>
FlagSet<Tag, Storage>&
operator|=(FlagSet<Tag, Storage>& a, FlagSet<Tag, Storage> b)
{
    a.code |= b.code;
    return a;
}
template<class Tag, class Storage>
FlagSet<Tag, Storage>
operator&(FlagSet<Tag, Storage> a, FlagSet<Tag, Storage> b)
{
    return FlagSet<Tag, Storage>(a.code & b.code);
}
template<class Tag, class Storage>
FlagSet<Tag, Storage>&
operator&=(FlagSet<Tag, Storage>& a, FlagSet<Tag, Storage> b)
{
    a.code &= b.code;
    return a;
}
template<class Tag, class Storage>
bool
operator==(FlagSet<Tag, Storage> a, FlagSet<Tag, Storage> b)
{
    return a.code == b.code;
}
template<class Tag, class Storage>
bool
operator!=(FlagSet<Tag, Storage> a, FlagSet<Tag, Storage> b)
{
    return a.code != b.code;
}
template<class Tag, class Storage>
bool
operator<(FlagSet<Tag, Storage> a, FlagSet<Tag, Storage> b)
{
    return a.code < b.code;
}
template<class Tag, class Storage>
FlagSet<Tag, Storage>
operator~(FlagSet<Tag, Storage> a)
{
    return FlagSet<Tag, Storage>(~a.code);
}

template<class Tag, class Storage>
Storage
raw_code(FlagSet<Tag, Storage> flags)
{
    return flags.code;
}

} // namespace alia

#define ALIA_DEFINE_FLAG_TYPE(TypePrefix)                                     \
    struct TypePrefix##FlagTag                                                \
    {                                                                         \
    };                                                                        \
    typedef alia::FlagSet<TypePrefix##FlagTag, std::uint32_t>                 \
        TypePrefix##FlagSet;

#define ALIA_DEFINE_FLAG(TypePrefix, code, name)                              \
    std::uint32_t const name##_CODE = code;                                   \
    alia::FlagSet<TypePrefix##FlagTag, std::uint32_t> const name(code);
