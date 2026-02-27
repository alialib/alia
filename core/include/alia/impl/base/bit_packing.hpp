#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <alia/abi/bits.h>

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
// 1. Define tag structures for your bit fields by inheriting from bitfield:
//
//    struct animation_bitfield : alia::bitfield<2> {};
//    struct flare_bitfield : alia::bitfield<3> {};
//    struct transition_bitfield : alia::bitfield<1> {};
//
// 2. Define a layout structure using these tags:
//
//    struct my_layout
//    {
//        animation_bitfield animation;
//        flare_bitfield flare;
//        transition_bitfield transition;
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
// 5. Read and write fields:
//
//    animation_bits = 2;
//    uint32_t value = animation_bits;
//
//    For single-bit fields, there are dedicated functions:
//
//    set_bit(transition_bit);
//    clear_bit(transition_bit);
//    toggle_bit(transition_bit);
//    bool in_transition = is_set(transition_bit);
//
// Note: The total size of all bit fields in a layout must not exceed 32 bits.

namespace alia {

template<unsigned Width>
struct bitfield
{
    static constexpr unsigned width = Width;
    static_assert(width < 32, "bitfield: `Width` must be less than 32 bits");
    char storage[width];
};

// `bitref` is a reference to a bit field.
template<class Tag>
struct bitref
{
    constexpr static unsigned width = Tag::width;
    uint32_t& storage;
    unsigned index;

    operator alia_bitref() const
    {
        return (alia_bitref) {&storage, index};
    }

    // Allow: "auto x = ref;"
    operator uint32_t() const
    {
        return (storage >> index) & ((1u << width) - 1);
    }

    // Allow: "ref = 5;"
    bitref&
    operator=(uint32_t value)
    {
        constexpr uint32_t mask = (1u << width) - 1;
        storage = (storage & ~(mask << index)) | ((value & mask) << index);
        return *this;
    }

    // Allow: "if (ref) ..."
    explicit
    operator bool() const
    {
        return this->operator uint32_t() != 0;
    }
};

// bitpack is used to store the actual bits for a layout.
template<class Layout>
struct bitpack
{
    using layout_t = Layout;
    uint32_t bits;

    static_assert(sizeof(Layout) <= 32, "bitpack: `Layout` exceeds 32 bits");
    // technically required to use offsetof()
    static_assert(
        std::is_standard_layout_v<Layout>,
        "bitpack: `Layout` must be a standard-layout type");
};

// nested_bitpack_ref is a reference to a nested pack of bits within a bitpack.
// This is used to reference members within composite layouts.
template<class Layout>
struct nested_bitpack_ref
{
    using layout_t = Layout;
    uint32_t& bits;
    unsigned offset;
};

// helper function to adjust the offset of a `nested_bitpack_ref` member
template<class Layout>
unsigned
adjust_offset(nested_bitpack_ref<Layout> const& pack, unsigned offset)
{
    return pack.offset + offset;
}

// (noop) helper function to adjust the offset of a `bitpack` member
template<class Layout>
unsigned
adjust_offset(bitpack<Layout> const&, unsigned offset)
{
    return offset;
}

#define ALIA_BITREF(pack, member)                                             \
    (alia::bitref<decltype(decltype(pack)::layout_t().member)>{               \
        (pack).bits,                                                          \
        alia::adjust_offset(                                                  \
            (pack), offsetof(decltype(pack)::layout_t, member))})

#define ALIA_NESTED_BITPACK(parent_pack, member)                              \
    (alia::nested_bitpack_ref<                                                \
        decltype(decltype(parent_pack)::layout_t().member)>{                  \
        (parent_pack).bits,                                                   \
        alia::adjust_offset(                                                  \
            (parent_pack),                                                    \
            offsetof(decltype(parent_pack)::layout_t, member))})

// Single-bit operations...

template<class Tag>
bool
is_set(bitref<Tag> ref)
{
    static_assert(Tag::width == 1, "is_set requires a one-bit field");
    return alia_bitref_is_set(ref);
}

template<class Tag>
void
clear_bit(bitref<Tag> ref)
{
    static_assert(Tag::width == 1, "clear_bit requires a one-bit field");
    alia_bitref_clear(ref);
}

template<class Tag>
void
set_bit(bitref<Tag> ref)
{
    static_assert(Tag::width == 1, "set_bit requires a one-bit field");
    alia_bitref_set(ref);
}

template<class Tag>
void
toggle_bit(bitref<Tag> ref)
{
    static_assert(Tag::width == 1, "toggle_bit requires a one-bit field");
    alia_bitref_toggle(ref);
}

} // namespace alia
