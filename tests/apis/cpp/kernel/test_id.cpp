#include <alia/kernel/id.hpp>

#include <doctest/doctest.h>

#include <stdint.h>
#include <string>
#include <vector>

using namespace alia;
using namespace alia::operators;

TEST_CASE("null id")
{
    CHECK(alia_id_view_is_null(null_id()));
    CHECK((null_id() == null_id()));
}

TEST_CASE("unit id")
{
    CHECK(alia_id_view_is_unit(unit_id()));
    CHECK_FALSE(alia_id_view_is_null(unit_id()));
    CHECK((unit_id() == unit_id()));
    CHECK((unit_id() == alia_id_view_unit()));
    CHECK((unit_id() != null_id()));
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

TEST_CASE("make_id_by_reference")
{
    int const x = 7;
    CHECK((make_id_by_reference(x) == make_id(7)));

    std::string const s = "abc";
    CHECK((make_id_by_reference(s) == alia_id_view_make_bytes("abc", 3u)));

    struct pod
    {
        int a;
        int b;
    };
    pod const p{1, 2};
    CHECK(
        (make_id_by_reference(p)
         == alia_id_view_make_bytes(
             reinterpret_cast<char const*>(&p), sizeof(pod))));
}

TEST_CASE("identifiable")
{
    CHECK((identifiable<bool>));
    CHECK((identifiable<int>));
    CHECK((identifiable<float>));
    CHECK((identifiable<double>));
    CHECK((identifiable<std::string>));

    struct pod
    {
        int a;
        int b;
    };
    CHECK((identifiable<pod>));

    struct no_id
    {
        std::string s;
    };
    CHECK_FALSE((identifiable<no_id>));
    CHECK_FALSE((identifiable<std::vector<int>>));
}

TEST_CASE("std::hash matches alia_id_view_hash")
{
    std::hash<id_view> const hash{};
    CHECK(hash(make_id(7)) == alia_id_view_hash(make_id(7)));
    CHECK(hash(make_id(7)) == hash(make_id(7)));
    CHECK(hash(make_id(7)) != hash(null_id()));
}

TEST_CASE("make_id_pair")
{
    alia_id_pair a_storage{};
    alia_id_pair a_again_storage{};
    alia_id_pair b_storage{};
    auto a = make_id_pair(a_storage, make_id(0), make_id(1));
    auto b = make_id_pair(b_storage, make_id(1), make_id(2));
    CHECK((a != b));
    CHECK((a == make_id_pair(a_again_storage, make_id(0), make_id(1))));
}
