#include <alia/flow/components.hpp>

#include <testing.hpp>

#include "traversal.hpp"

using namespace alia;

TEST_CASE("component_data", "[signals][state]")
{
    component_data<int> s;
    REQUIRE(!s.is_initialized());
    REQUIRE(s.version() == 0);
    s.set(1);
    REQUIRE(s.is_initialized());
    REQUIRE(s.version() == 1);
    REQUIRE(s.get() == 1);
    s.nonconst_ref() = 4;
    REQUIRE(s.is_initialized());
    REQUIRE(s.version() == 2);
    REQUIRE(s.get() == 4);
}
