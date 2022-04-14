#include <alia/actions/basic.hpp>

#include <testing.hpp>

using namespace alia;

TEST_CASE("unready action", "[actions][basic]")
{
    REQUIRE(!actions::unready().is_ready());
    REQUIRE(!actions::unready<int>().is_ready());
}

TEST_CASE("noop action", "[actions][basic]")
{
    REQUIRE(actions::noop().is_ready());
    perform_action(actions::noop());
    REQUIRE(actions::noop<int>().is_ready());
    perform_action(actions::noop<int>(), 1);
}

TEST_CASE("callbacks", "[actions][basic]")
{
    int x = 0;
    auto a = callback([&](int y, int z) { x = y + z; });
    perform_action(a, 1, 2);
    REQUIRE(x == 3);

    bool ready = false;
    auto b = callback([&]() { return ready; }, [&](int y) { x += y; });
    REQUIRE(!b.is_ready());
    ready = true;
    REQUIRE(b.is_ready());
    perform_action(b, 1);
    REQUIRE(x == 4);
}
