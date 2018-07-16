#include <alia/common.hpp>

#include <catch.hpp>

using namespace alia;

TEST_CASE("exception", "[common]")
{
    exception e("just a test");
    REQUIRE(e.what() == std::string("just a test"));
    e.add_context("in here");
    REQUIRE(e.what() == std::string("just a test\nin here"));
}

TEST_CASE("invoke_hash", "[common]")
{
    REQUIRE(invoke_hash(17) == std::hash<int>()(17));
}

TEST_CASE("combine_hashes", "[common]")
{
    // It's hard to establish specific requirements for this, but these seem
    // reasonable as basic requirements.

    // Argument order matters.
    REQUIRE(combine_hashes(17, 12) != combine_hashes(12, 17));

    // The first argument matters.
    REQUIRE(combine_hashes(17, 12) != combine_hashes(16, 12));

    // The second argument matters.
    REQUIRE(combine_hashes(17, 12) != combine_hashes(17, 11));
}
