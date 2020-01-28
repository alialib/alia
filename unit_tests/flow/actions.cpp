#include <alia/flow/actions.hpp>

#include <alia/signals/basic.hpp>

#include <catch.hpp>

using namespace alia;

TEST_CASE("copy actions", "[flow][actions]")
{
    int x = 1;
    REQUIRE(!(empty<int>() <<= empty<int>()).is_ready());
    REQUIRE(!(direct(x) <<= empty<int>()).is_ready());
    REQUIRE(!(empty<int>() <<= direct(x)).is_ready());
    auto a = direct(x) <<= value(2);
    REQUIRE(a.is_ready());
    REQUIRE(x == 1);
    perform_action(a);
    REQUIRE(x == 2);
    auto liberal = direct(x) <<= 4;
    REQUIRE(liberal.is_ready());
    perform_action(liberal);
    REQUIRE(x == 4);
}

#define TEST_COMPOUND_ASSIGNMENT_OPERATOR(op, normal_form)                     \
    {                                                                          \
        int x = 21;                                                            \
        REQUIRE(!(empty<int>() op empty<int>()).is_ready());                   \
        REQUIRE(!(direct(x) op empty<int>()).is_ready());                      \
        REQUIRE(!(empty<int>() op direct(x)).is_ready());                      \
        auto a = direct(x) op value(7);                                        \
        REQUIRE(a.is_ready());                                                 \
        REQUIRE(x == 21);                                                      \
        perform_action(a);                                                     \
        REQUIRE(x == (21 normal_form 7));                                      \
        x = 2;                                                                 \
        auto liberal = direct(x) op 6;                                         \
        REQUIRE(liberal.is_ready());                                           \
        perform_action(liberal);                                               \
        REQUIRE(x == (2 normal_form 6));                                       \
    }

TEST_CASE("compound assignment action operators", "[flow][actions]")
{
    TEST_COMPOUND_ASSIGNMENT_OPERATOR(+=, +)
    TEST_COMPOUND_ASSIGNMENT_OPERATOR(-=, -)
    TEST_COMPOUND_ASSIGNMENT_OPERATOR(*=, *)
    TEST_COMPOUND_ASSIGNMENT_OPERATOR(/=, /)
    TEST_COMPOUND_ASSIGNMENT_OPERATOR(^=, ^)
    TEST_COMPOUND_ASSIGNMENT_OPERATOR(%=, %)
    TEST_COMPOUND_ASSIGNMENT_OPERATOR(&=, &)
    TEST_COMPOUND_ASSIGNMENT_OPERATOR(|=, |)
}

#undef TEST_BY_ONE_OPERATOR

#define TEST_BY_ONE_OPERATOR(op, normal_form)                                  \
    {                                                                          \
        int x = 21;                                                            \
        REQUIRE(!(empty<int>() op).is_ready());                                \
        REQUIRE(!(op empty<int>()).is_ready());                                \
        {                                                                      \
            auto a = direct(x) op;                                             \
            REQUIRE(a.is_ready());                                             \
            REQUIRE(x == 21);                                                  \
            perform_action(a);                                                 \
            REQUIRE(x == (21 normal_form 1));                                  \
        }                                                                      \
        {                                                                      \
            auto a = op direct(x);                                             \
            REQUIRE(a.is_ready());                                             \
            perform_action(a);                                                 \
            REQUIRE(x == (21 normal_form 2));                                  \
        }                                                                      \
    }

TEST_CASE("increment/decrement operators", "[flow][actions]")
{
    TEST_BY_ONE_OPERATOR(++, +)
    TEST_BY_ONE_OPERATOR(--, -)
}

#undef TEST_COMPOUND_ASSIGNMENT_OPERATOR

TEST_CASE("sequenced actions", "[flow][actions]")
{
    int x = 1, y = 2;
    auto a = empty<int>() <<= empty<int>();
    auto b = direct(x) <<= value(2);
    auto c = direct(y) <<= value(3);
    REQUIRE(!(a, b).is_ready());
    REQUIRE((b, c, b).is_ready());
    perform_action((b, c));
    REQUIRE(x == 2);
    REQUIRE(y == 3);
}

TEST_CASE("action_ref", "[flow][actions]")
{
    int x = 1;
    auto a = empty<int>() <<= empty<int>();
    auto b = direct(x) <<= value(2);

    action_ref<> r = b;
    REQUIRE(r.is_ready());
    perform_action(r);
    REQUIRE(x == 2);

    x = 1;

    action_ref<> s = r;
    REQUIRE(s.is_ready());
    REQUIRE(action_is_ready(s));
    perform_action(s);
    REQUIRE(x == 2);

    s = a;
    REQUIRE(!s.is_ready());
    REQUIRE(!action_is_ready(s));
}

static void
f(action<> a)
{
    REQUIRE(!a.is_ready());
}

TEST_CASE("action parameter passing", "[flow][actions]")
{
    auto a = empty<int>() <<= empty<int>();
    f(a);
}

TEST_CASE("toggle action", "[flow][actions]")
{
    bool x = false;
    {
        auto a = make_toggle_action(direct(x));
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(x);
    }
    {
        auto a = make_toggle_action(direct(x));
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(!x);
    }

    {
        auto a = make_toggle_action(empty<bool>());
        REQUIRE(!a.is_ready());
    }
}

TEST_CASE("push_back action", "[flow][actions]")
{
    auto x = std::vector<int>{1, 2};
    {
        auto a = make_push_back_action(direct(x), value(3));
        REQUIRE(a.is_ready());
        perform_action(a);
        REQUIRE(x == (std::vector<int>{1, 2, 3}));
    }

    {
        auto a = make_push_back_action(direct(x), empty<int>());
        REQUIRE(!a.is_ready());
    }
    {
        auto a = make_push_back_action(empty<std::vector<int>>(), value(3));
        REQUIRE(!a.is_ready());
    }
}

TEST_CASE("lambda actions", "[flow][actions]")
{
    int x = 0;
    auto a = lambda_action(always_ready, [&](int y, int z) { x = y + z; });
    perform_action(a, 1, 2);
    REQUIRE(x == 3);

    bool ready = false;
    auto b = lambda_action([&]() { return ready; }, [&](int y) { x += y; });
    REQUIRE(!b.is_ready());
    ready = true;
    REQUIRE(b.is_ready());
    perform_action(b, 1);
    REQUIRE(x == 4);
}

TEST_CASE("parameterized actions", "[flow][actions]")
{
    int my_int = 0;
    auto assign = [&my_int](int x) { my_int = x; };
    auto a = parameterized_action(assign, empty<int>());
    REQUIRE(!action_is_ready(a));
    auto b = parameterized_action(assign, value(1));
    REQUIRE(action_is_ready(b));
    perform_action(b);
    REQUIRE(my_int == 1);
}
