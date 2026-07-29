#include <alia/kernel/id.hpp>

#include <doctest/doctest.h>

#include <stdint.h>

using namespace alia;
using namespace alia::operators;

TEST_CASE("null id")
{
    CHECK(alia_id_view_is_null(null_id()));
    CHECK((null_id() == null_id()));
}

TEST_CASE("make_id bool and integrals")
{
    CHECK((make_id(false) == alia_id_view_make_u32(0)));
    CHECK((make_id(true) == alia_id_view_make_u32(1)));

    CHECK((make_id(int32_t{-7}) == alia_id_view_make_i32(-7)));
    CHECK((make_id(uint32_t{42}) == alia_id_view_make_u32(42)));
    CHECK((make_id(int64_t{-9}) == alia_id_view_make_i64(-9)));
    CHECK((make_id(uint64_t{99}) == alia_id_view_make_u64(99)));

    CHECK((make_id(1) != make_id(2)));
    CHECK((make_id(true) != make_id(false)));
}

TEST_CASE("make_id float and double")
{
    float const a = 1.5f;
    float const b = 1.5f;
    float const c = 2.5f;
    CHECK((make_id(a) == make_id(b)));
    CHECK((make_id(a) != make_id(c)));

    double const d = 3.25;
    double const e = 3.25;
    CHECK((make_id(d) == make_id(e)));
    CHECK((make_id(d) != make_id(4.0)));
}

TEST_CASE("make_pointer_id")
{
    static char const lit_a[] = "hello";
    static char const lit_b[] = "hello";
    CHECK((make_pointer_id(lit_a) == alia_id_view_make_pointer(lit_a)));
    // Distinct static arrays are distinct pointer identities even with the
    // same contents.
    CHECK((make_pointer_id(lit_a) != make_pointer_id(lit_b)));
}

TEST_CASE("std::hash matches alia_id_view_hash")
{
    std::hash<id_view> const hash{};
    CHECK(hash(make_id(7)) == alia_id_view_hash(make_id(7)));
    CHECK(hash(make_id(7)) == hash(make_id(7)));
    CHECK(hash(make_id(7)) != hash(null_id()));
}
