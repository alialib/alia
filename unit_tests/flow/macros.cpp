#define ALIA_LOWERCASE_MACROS
#include <alia/flow/macros.hpp>

#include <sstream>

#include <alia/signals/basic.hpp>

#include <boost/lexical_cast.hpp>

#include <catch.hpp>

TEST_CASE("condition_is_true/false/etc", "[flow][macros]")
{
    REQUIRE(condition_is_true(value(true)));
    REQUIRE(!condition_is_true(value(false)));
    REQUIRE(!condition_is_true(empty<bool>()));

    REQUIRE(condition_is_false(value(false)));
    REQUIRE(!condition_is_false(value(true)));
    REQUIRE(!condition_is_false(empty<bool>()));
}
