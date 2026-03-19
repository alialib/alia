#include <alia/impl/base/bit_packing.hpp>

#include <doctest/doctest.h>

using namespace alia;

TEST_CASE("bitpack")
{
    struct bitfield_a : bitfield<2>
    {
    };
    struct bitfield_b : bitfield<3>
    {
    };
    struct bitfield_c : bitfield<1>
    {
    };

    struct test_layout
    {
        bitfield_a a;
        bitfield_b b;
        bitfield_c c;
    };

    bitpack<test_layout> pack{0};

    // Get references to the individual fields.

    auto a_ref = ALIA_BITREF(pack, a);
    REQUIRE(a_ref.index == 0);
    REQUIRE(&a_ref.storage == &pack.bits);
    REQUIRE(a_ref.width == 2);
    static_assert(std::is_same_v<decltype(a_ref), bitref<bitfield_a>>);

    auto b_ref = ALIA_BITREF(pack, b);
    REQUIRE(b_ref.index == 2);
    REQUIRE(&b_ref.storage == &pack.bits);
    REQUIRE(b_ref.width == 3);
    static_assert(std::is_same_v<decltype(b_ref), bitref<bitfield_b>>);

    auto c_ref = ALIA_BITREF(pack, c);
    REQUIRE(c_ref.index == 5);
    REQUIRE(&c_ref.storage == &pack.bits);
    REQUIRE(c_ref.width == 1);
    static_assert(std::is_same_v<decltype(c_ref), bitref<bitfield_c>>);

    // Test reading and writing individual fields.

    a_ref = 2;
    REQUIRE(a_ref == 2);
    REQUIRE(pack.bits == 0b000'0010);

    b_ref = 5;
    REQUIRE(b_ref == 5);
    REQUIRE(pack.bits == 0b0001'0110);

    // Test single-bit operations.

    set_bit(c_ref);
    REQUIRE(is_set(c_ref) == true);
    REQUIRE(pack.bits == 0b0011'0110);

    clear_bit(c_ref);
    REQUIRE(is_set(c_ref) == false);
    REQUIRE(pack.bits == 0b0001'0110);

    toggle_bit(c_ref);
    REQUIRE(is_set(c_ref) == true);
    REQUIRE(pack.bits == 0b0011'0110);

    // Test overwriting fields.
    a_ref = 3;
    REQUIRE(a_ref == 3);
    b_ref = 5;
    REQUIRE(b_ref == 5);
    c_ref = 1;
    REQUIRE(c_ref == 1);

    // Test writing values that exceed the field size.
    // 5 is 101 in binary, but only 01 should be stored.
    a_ref = 5;
    REQUIRE(a_ref == 1);
    b_ref = 5;
    REQUIRE(b_ref == 5);

    // Test that fields don't interfere with each other.
    b_ref = 7;
    REQUIRE(a_ref == 1);
    REQUIRE(b_ref == 7);
    REQUIRE(c_ref == 1);

    // Test clearing fields.
    b_ref = 0;
    REQUIRE(b_ref == 0);
    REQUIRE(a_ref == 1);
    REQUIRE(c_ref == 1);
}

TEST_CASE("bitpack subpacks")
{
    struct inner_layout
    {
        bitfield<2> x;
        bitfield<3> y;
    };

    struct outer_layout
    {
        bitfield<1> a;
        inner_layout inner;
        bitfield<2> b;
    };

    bitpack<outer_layout> pack;
    pack.bits = 0;

    auto a_ref = ALIA_BITREF(pack, a);
    auto b_ref = ALIA_BITREF(pack, b);

    auto inner_ref = ALIA_NESTED_BITPACK(pack, inner);

    auto x_ref = ALIA_BITREF(inner_ref, x);
    auto y_ref = ALIA_BITREF(inner_ref, y);

    // Test writing to subpack fields.
    a_ref = 1;
    x_ref = 2;
    y_ref = 5;
    b_ref = 3;

    // Verify the values.
    REQUIRE(a_ref == 1);
    REQUIRE(x_ref == 2);
    REQUIRE(y_ref == 5);
    REQUIRE(b_ref == 3);

    // Verify the overall bitpack.
    REQUIRE(pack.bits == 0b1110'1101);

    // Test overwriting subpack fields.
    x_ref = 3;
    y_ref = 2;

    // Verify the new values.
    REQUIRE(x_ref == 3);
    REQUIRE(y_ref == 2);

    // Verify the overall bitpack again
    REQUIRE(pack.bits == 0b1101'0111);
}

