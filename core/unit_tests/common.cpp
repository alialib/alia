#include <alia/core/common.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace alia;

TEST_CASE("exception", "[common]")
{
    exception e("just a test");
    REQUIRE(e.what() == std::string("just a test"));
    e.add_context("in here");
    REQUIRE(e.what() == std::string("just a test\nin here"));
}

struct aggregatable_struct
{
    int a;
    std::string b;
};

TEST_CASE("ALIA_AGGREGATOR", "[common]")
{
    std::function<aggregatable_struct(int, std::string)> f;
    f = ALIA_AGGREGATOR(aggregatable_struct);
    auto x = f(4, "foo");
    REQUIRE(x.a == 4);
    REQUIRE(x.b == "foo");
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

struct uncaught_exception_tester
{
    uncaught_exception_tester(bool* result) : result(result)
    {
    }

    ~uncaught_exception_tester()
    {
        *result = detector.detect();
    }

    bool* result;
    uncaught_exception_detector detector;
};

TEST_CASE("uncaught_exception_detector", "[common]")
{
    bool detected = false;
    try
    {
        uncaught_exception_tester tester(&detected);
        throw nullptr;
    }
    catch (...)
    {
    }
    REQUIRE(detected);

    {
        uncaught_exception_tester tester(&detected);
    }
    REQUIRE(!detected);
}
