#include <alia/common.hpp>

#include <catch2/catch.hpp>

using namespace alia;

TEST_CASE("exception", "[common]")
{
    exception e("just a test");
    REQUIRE(e.what() == std::string("just a test"));
    e.add_context("in here");
    REQUIRE(e.what() == std::string("just a test\nin here"));
}
