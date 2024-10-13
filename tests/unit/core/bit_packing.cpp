#include <alia/core/bit_packing.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace alia;

TEST_CASE("bitpack", "[bit_packing]")
{
    struct bit_field_a : bit_field<2>
    {
    };
    struct bit_field_b : bit_field<3>
    {
    };
    struct bit_field_c : bit_field<1>
    {
    };

    struct test_layout
    {
        bit_field_a a;
        bit_field_b b;
        bit_field_c c;
    };

    bitpack<test_layout> pack{0};

    // Get references to the individual fields.

    auto a_ref = ALIA_BITREF(pack, a);
    REQUIRE(a_ref.index == 0);
    REQUIRE(&a_ref.storage == &pack.bits);
    REQUIRE(a_ref.size == 2);
    static_assert(std::is_same_v<decltype(a_ref), bitref<bit_field_a>>);

    auto b_ref = ALIA_BITREF(pack, b);
    REQUIRE(b_ref.index == 2);
    REQUIRE(&b_ref.storage == &pack.bits);
    REQUIRE(b_ref.size == 3);
    static_assert(std::is_same_v<decltype(b_ref), bitref<bit_field_b>>);

    auto c_ref = ALIA_BITREF(pack, c);
    REQUIRE(c_ref.index == 5);
    REQUIRE(&c_ref.storage == &pack.bits);
    REQUIRE(c_ref.size == 1);
    static_assert(std::is_same_v<decltype(c_ref), bitref<bit_field_c>>);

    // Test reading and writing individual fields.

    write_bitref(a_ref, 2);
    REQUIRE(read_bitref(a_ref) == 2);
    REQUIRE(pack.bits == 0b0000010);

    write_bitref(b_ref, 5);
    REQUIRE(read_bitref(b_ref) == 5);
    REQUIRE(pack.bits == 0b00010110);

    // Test single-bit operations.

    set_bit(c_ref);
    REQUIRE(is_set(c_ref) == true);
    REQUIRE(pack.bits == 0b00110110);

    clear_bit(c_ref);
    REQUIRE(is_set(c_ref) == false);
    REQUIRE(pack.bits == 0b00010110);

    toggle_bit(c_ref);
    REQUIRE(is_set(c_ref) == true);
    REQUIRE(pack.bits == 0b00110110);

    // Test overwriting fields.
    write_bitref(a_ref, 3);
    REQUIRE(read_bitref(a_ref) == 3);
    write_bitref(b_ref, 5);
    REQUIRE(read_bitref(b_ref) == 5);
    write_bitref(c_ref, 1);
    REQUIRE(read_bitref(c_ref) == 1);

    // Test writing values that exceed the field size.
    // 5 is 101 in binary, but only 01 should be stored.
    write_bitref(a_ref, 5);
    REQUIRE(read_bitref(a_ref) == 1);
    write_bitref(b_ref, 5);
    REQUIRE(read_bitref(b_ref) == 5);

    // Test that fields don't interfere with each other.
    write_bitref(b_ref, 7);
    REQUIRE(read_bitref(a_ref) == 1);
    REQUIRE(read_bitref(b_ref) == 7);
    REQUIRE(read_bitref(c_ref) == 1);

    // Test clearing fields.
    write_bitref(b_ref, 0);
    REQUIRE(read_bitref(b_ref) == 0);
    REQUIRE(read_bitref(a_ref) == 1);
    REQUIRE(read_bitref(c_ref) == 1);
}

TEST_CASE("bitpack subpacks", "[bit_packing]")
{
    struct inner_layout
    {
        bit_field<2> x;
        bit_field<3> y;
    };

    struct outer_layout
    {
        bit_field<1> a;
        inner_layout inner;
        bit_field<2> b;
    };

    bitpack<outer_layout> pack;
    pack.bits = 0;

    auto a_ref = ALIA_BITREF(pack, a);
    auto b_ref = ALIA_BITREF(pack, b);

    auto inner_ref = ALIA_NESTED_BITPACK(pack, inner);
    REQUIRE(inner_ref.offset == 1);

    auto x_ref = ALIA_BITREF(inner_ref, x);
    auto y_ref = ALIA_BITREF(inner_ref, y);

    // Test writing to subpack fields.
    write_bitref(a_ref, 1);
    write_bitref(x_ref, 2);
    write_bitref(y_ref, 5);
    write_bitref(b_ref, 3);

    // Verify the values.
    REQUIRE(read_bitref(a_ref) == 1);
    REQUIRE(read_bitref(x_ref) == 2);
    REQUIRE(read_bitref(y_ref) == 5);
    REQUIRE(read_bitref(b_ref) == 3);

    // Verify the overall bitpack.
    REQUIRE(pack.bits == 0b11101101);

    // Test overwriting subpack fields.
    write_bitref(x_ref, 3);
    write_bitref(y_ref, 2);

    // Verify the new values.
    REQUIRE(read_bitref(x_ref) == 3);
    REQUIRE(read_bitref(y_ref) == 2);

    // Verify the overall bitpack again
    REQUIRE(pack.bits == 0b11010111);
}
