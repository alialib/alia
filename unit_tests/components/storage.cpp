#include <alia/components/storage.hpp>

#include <catch.hpp>

using namespace alia;

TEST_CASE("any_value basics", "[components][storage]")
{
    any_value a(int(17));
    REQUIRE(any_cast<int>(&a));
    REQUIRE(*any_cast<int>(&a) == 17);
    REQUIRE(!any_cast<bool>(&a));

    any_value b(std::move(a));
    REQUIRE(*any_cast<int>(&b) == 17);

    any_value const c = b;
    REQUIRE(*any_cast<int>(&c) == 17);

    any_value d;
    REQUIRE(!any_cast<int>(&d));
    std::string xyz("xyz");
    d = xyz;
    REQUIRE(*any_cast<std::string>(&d) == "xyz");

    any_value e(xyz);
    REQUIRE(*any_cast<std::string>(&e) == "xyz");
    e = b;
    REQUIRE(*any_cast<int>(&e) == 17);
    REQUIRE(*any_cast<int>(&b) == 17);

    any_value f;
    f = std::move(e);
    REQUIRE(*any_cast<int>(&f) == 17);

    any_value g;
    std::string abc("abc");
    g = std::move(abc);
    REQUIRE(*any_cast<std::string>(&g) == "abc");

    swap(f, g);
    REQUIRE(*any_cast<int>(&g) == 17);
    REQUIRE(*any_cast<std::string>(&f) == "abc");
}
