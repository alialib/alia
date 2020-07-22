#include <alia/common.hpp>

#include <testing.hpp>

using namespace alia;

TEST_CASE("exception", "[common]")
{
    exception e("just a test");
    REQUIRE(e.what() == std::string("just a test"));
    e.add_context("in here");
    REQUIRE(e.what() == std::string("just a test\nin here"));
}

int
invoke_function_views(function_view<int(bool)> a, function_view<int(int)> b)
{
    return a(true) + b(12);
}

TEST_CASE("function_view", "[common]")
{
    REQUIRE(
        invoke_function_views(
            [](bool x) { return x ? 1 : 2; }, [](int x) { return x / 2; })
        == 7);
    REQUIRE(
        invoke_function_views(
            [](bool x) { return x ? 3 : 0; }, [](int x) { return x * 2; })
        == 27);
}

TEST_CASE("is_invocable", "[common]")
{
    auto f = [](double, int) {};
    static_assert(
        alia::is_invocable<decltype(f), double, int>::value, "invocable");
    static_assert(
        !alia::is_invocable<decltype(f), double, std::string>::value,
        "type mismatch");
    static_assert(
        !alia::is_invocable<decltype(f), std::string>::value,
        "not enough arguments");
    static_assert(
        !alia::is_invocable<decltype(f), int, int, std::string>::value,
        "too many arguments");
}
