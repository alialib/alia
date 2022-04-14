#include <alia/actions/library.hpp>

#include <alia/signals/basic.hpp>
#include <alia/signals/operators.hpp>

#include <move_testing.hpp>
#include <testing.hpp>

using namespace alia;

TEST_CASE("toggle action", "[actions][library]")
{
    bool x = false;
    {
        auto a = actions::toggle(direct(x));
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(x);
    }
    {
        auto a = actions::toggle(direct(x));
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(!x);
    }

    {
        auto a = actions::toggle(empty<bool>());
        REQUIRE(!a.is_ready());
    }
}

TEST_CASE("push_back action", "[actions][library]")
{
    auto x = std::vector<int>{1, 2};
    {
        auto a = actions::push_back(direct(x));
        REQUIRE(a.is_ready());
        perform_action(a, 3);
        REQUIRE(x == (std::vector<int>{1, 2, 3}));
    }
    {
        auto a = actions::push_back(direct(x)) << 4;
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(x == (std::vector<int>{1, 2, 3, 4}));
    }

    {
        auto a = actions::push_back(empty<std::vector<int>>());
        REQUIRE(!a.is_ready());
    }

    {
        auto a = actions::push_back(empty<std::vector<int>>());
        REQUIRE(!a.is_ready());
    }
}

TEST_CASE("push_back movable", "[actions][library]")
{
    auto x = std::vector<movable_object>{movable_object(1), movable_object(2)};
    {
        auto a = actions::push_back(direct(x)) << value(movable_object(3));
        REQUIRE(a.is_ready());
        copy_count = 0;
        perform_action(a);
        REQUIRE(copy_count == 0);
        REQUIRE(
            x
            == (std::vector<movable_object>{
                movable_object(1), movable_object(2), movable_object(3)}));
    }
}

TEST_CASE("erase_index action", "[actions][library]")
{
    auto x = std::vector<int>{1, 2, 3, 4};
    {
        auto a = actions::erase_index(direct(x), 2);
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(x == (std::vector<int>{1, 2, 4}));
    }
    {
        auto a = actions::erase_index(direct(x), value(0));
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(x == (std::vector<int>{2, 4}));
    }

    {
        auto a = actions::erase_index(direct(x), empty<size_t>());
        REQUIRE(!a.is_ready());
    }
    {
        auto a = actions::erase_index(empty<std::vector<int>>(), value(0));
        REQUIRE(!a.is_ready());
    }
    {
        auto a = actions::erase_index(
            fake_writability(value(std::vector<int>(1, 2))), value(0));
        REQUIRE(!a.is_ready());
    }
}

TEST_CASE("erase_index movement", "[actions][library]")
{
    auto x = std::vector<movable_object>{
        movable_object(1), movable_object(2), movable_object(3)};
    {
        auto a = actions::erase_index(direct(x), 1);
        REQUIRE(a.is_ready());
        copy_count = 0;
        perform_action(a);
        REQUIRE(copy_count == 0);
        REQUIRE(
            x
            == (std::vector<movable_object>{
                movable_object(1), movable_object(3)}));
    }
}

TEST_CASE("erase_key action", "[actions][library]")
{
    auto x = std::map<int, int>{{1, 2}, {2, 4}, {3, 6}};
    {
        auto a = actions::erase_key(direct(x), 2);
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(x == (std::map<int, int>{{1, 2}, {3, 6}}));
    }
    {
        auto a = actions::erase_key(direct(x), value(1));
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(x == (std::map<int, int>{{3, 6}}));
    }
    {
        auto a = actions::erase_key(direct(x), empty<int>());
        REQUIRE(!a.is_ready());
    }
    {
        auto a = actions::erase_key(empty<std::map<int, int>>(), value(0));
        REQUIRE(!a.is_ready());
    }
    {
        auto a = actions::erase_key(
            fake_writability(value(std::map<int, int>{{1, 2}})), value(0));
        REQUIRE(!a.is_ready());
    }
}
TEST_CASE("erase_key movement", "[actions][library]")
{
    auto x = std::map<int, movable_object>{
        {1, movable_object(1)},
        {2, movable_object(2)},
        {3, movable_object(3)}};
    {
        auto a = actions::erase_key(direct(x), 1);
        REQUIRE(a.is_ready());
        copy_count = 0;
        perform_action(a);
        REQUIRE(copy_count == 0);
        REQUIRE(
            x
            == (std::map<int, movable_object>{
                {2, movable_object(2)}, {3, movable_object(3)}}));
    }
}

TEST_CASE("actions::apply", "[actions][library]")
{
    auto add = [](int x, int y) { return x + y; };
    int x = 0;
    auto a = actions::apply(add, direct(x), value(1));
    REQUIRE(a.is_ready());
    perform_action(a);
    REQUIRE(x == 1);
}
