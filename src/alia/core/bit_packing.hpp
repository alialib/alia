#ifndef ALIA_CORE_BIT_PACKING_HPP
#define ALIA_CORE_BIT_PACKING_HPP

#include <cstdint>

// This file provides utilities for composing bit fields into a single uint32_t
// value.
//
// Its advantages over C-style bit fields include:
// - Structured composition within the uint32_t
// - Type-safe references to individual bit fields
// - Compile-time size checks
//
// Usage:
//
// 1. Define tag structures for your bit fields by inheriting from bit_field:
//
//    struct animation_bit_field : alia::bit_field<2> {};
//    struct flare_bit_field : alia::bit_field<3> {};
//    struct transition_bit_field : alia::bit_field<1> {};
//
// 2. Define a layout structure using these tags:
//
//    struct my_layout
//    {
//        animation_bit_field animation;
//        flare_bit_field flare;
//        transition_bit_field transition;
//    };
//
// 3. Create a bitpack instance wherever you want to store the bits:
//
//    alia::bitpack<my_layout> pack;
//
// 4. Get a reference to a field using the ALIA_BITREF macro:
//
//    auto animation_bits = ALIA_BITREF(pack, animation);
//    auto flare_bits = ALIA_BITREF(pack, flare);
//    auto transition_bit = ALIA_BITREF(pack, transition);
//
// 5. Read and write fields using the bitref functions:
//
//    write_bitref(animation_bits, 2);
//    auto value = read_bitref(animation_bits);
//
// 6. For single-bit fields, there are convenience functions:
//
//    set_bit(ALIA_BITREF(pack, transition));
//    clear_bit(ALIA_BITREF(pack, transition));
//    toggle_bit(ALIA_BITREF(pack, transition));
//    bool in_transition = is_set(ALIA_BITREF(pack, transition));
//
// Note: The total size of all bit fields in a layout must not exceed 32 bits.

namespace alia {

template<unsigned Size, class Tag = void>
struct bit_field
{
    static constexpr unsigned size = Size;
    using tag = Tag;
    char storage[size];
};

// bitref is a reference to a bit field.
template<class Tag>
struct bitref
{
    constexpr static unsigned size = Tag::size;
    uint32_t& storage;
    unsigned index;
};

// bitpack is used to store the actual bits for a layout.
template<class Layout>
struct bitpack
{
    using layout_t = Layout;
    uint32_t bits;
    static_assert(sizeof(Layout) <= 32);
};

template<class Layout>
unsigned
adjust_offset(bitpack<Layout>&, unsigned offset)
{
    return offset;
}

// bitpack_ref is a reference to a bitpack, with an offset.
// This is used to reference members within composite layouts.
template<class Layout>
struct bitpack_ref
{
    using layout_t = Layout;
    uint32_t& bits;
    unsigned offset;
};

// Get the offset of a bitpack_ref member.
template<class Layout>
unsigned
adjust_offset(bitpack_ref<Layout>& pack, unsigned offset)
{
    return pack.offset + offset;
}

#define ALIA_BITREF(pack, member)                                             \
    (alia::bitref<decltype(decltype(pack)::layout_t().member)>{               \
        (pack).bits,                                                          \
        adjust_offset(pack, offsetof(decltype(pack)::layout_t, member))})

#define ALIA_NESTED_BITPACK(parent_pack, member)                              \
    (alia::bitpack_ref<decltype(decltype(parent_pack)::layout_t().member)>{   \
        (parent_pack).bits,                                                   \
        adjust_offset(                                                        \
            parent_pack, offsetof(decltype(parent_pack)::layout_t, member))})

// General-purpose bitref operations...

template<class Tag>
unsigned
read_bitref(bitref<Tag> const& ref)
{
    constexpr uint32_t mask = (1u << Tag::size) - 1;
    return (ref.storage >> ref.index) & mask;
}

template<class Tag>
void
write_bitref(bitref<Tag>& ref, uint32_t value)
{
    constexpr uint32_t mask = (1u << Tag::size) - 1;
    ref.storage
        = (ref.storage & ~(mask << ref.index)) | ((value & mask) << ref.index);
}

// Single-bit operations...

template<class Tag>
bool
is_set(bitref<Tag>& ref)
{
    static_assert(Tag::size == 1, "is_set requires a single-bit field");
    return (ref.storage & (1u << ref.index)) != 0;
}

template<class Tag>
void
clear_bit(bitref<Tag>& ref)
{
    static_assert(Tag::size == 1, "clear_bit requires a single-bit field");
    ref.storage &= ~(1u << ref.index);
}

template<class Tag>
void
set_bit(bitref<Tag>& ref)
{
    static_assert(Tag::size == 1, "set_bit requires a single-bit field");
    ref.storage |= (1u << ref.index);
}

template<class Tag>
void
toggle_bit(bitref<Tag>& ref)
{
    static_assert(Tag::size == 1, "toggle_bit requires a single-bit field");
    ref.storage ^= (1u << ref.index);
}
} // namespace alia

#endif
