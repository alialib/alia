#include <alia/core/actions/operators.hpp>

#include <alia/core/actions/adaptors.hpp>
#include <alia/core/signals/basic.hpp>
#include <alia/core/signals/operators.hpp>

#include <move_testing.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace alia;

TEST_CASE("copy actions", "[actions][operators]")
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

#define TEST_COMPOUND_ASSIGNMENT_OPERATOR(op, normal_form)                    \
    {                                                                         \
        int x = 21;                                                           \
        REQUIRE(!(empty<int>() op empty<int>()).is_ready());                  \
        REQUIRE(!(direct(x) op empty<int>()).is_ready());                     \
        REQUIRE(!(empty<int>() op direct(x)).is_ready());                     \
        auto a = direct(x) op value(7);                                       \
        REQUIRE(a.is_ready());                                                \
        REQUIRE(x == 21);                                                     \
        perform_action(a);                                                    \
        REQUIRE(x == (21 normal_form 7));                                     \
        x = 2;                                                                \
        auto liberal = direct(x) op 6;                                        \
        REQUIRE(liberal.is_ready());                                          \
        perform_action(liberal);                                              \
        REQUIRE(x == (2 normal_form 6));                                      \
    }

TEST_CASE("compound assignment action operators", "[actions][operators]")
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

#undef TEST_COMPOUND_ASSIGNMENT_OPERATOR

#define TEST_BY_ONE_OPERATOR(op, normal_form)                                 \
    {                                                                         \
        int x = 21;                                                           \
        REQUIRE(!(empty<int>() op).is_ready());                               \
        REQUIRE(!(op empty<int>()).is_ready());                               \
        {                                                                     \
            auto a = direct(x) op;                                            \
            REQUIRE(a.is_ready());                                            \
            REQUIRE(x == 21);                                                 \
            perform_action(a);                                                \
            REQUIRE(x == (21 normal_form 1));                                 \
        }                                                                     \
        {                                                                     \
            auto a = op direct(x);                                            \
            REQUIRE(a.is_ready());                                            \
            perform_action(a);                                                \
            REQUIRE(x == (21 normal_form 2));                                 \
        }                                                                     \
    }

TEST_CASE("increment/decrement operators", "[actions][operators]")
{
    TEST_BY_ONE_OPERATOR(++, +)
    TEST_BY_ONE_OPERATOR(--, -)
}

#undef TEST_BY_ONE_OPERATOR

TEST_CASE("combined actions", "[actions][operators]")
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

TEST_CASE("parameterized combined action", "[actions][operators]")
{
    int x = 0, y = 0;
    auto a = callback([&](int n) { x += n; });
    auto b = callback([&](int n) { y -= n; });
    perform_action((a, b), 4);
    REQUIRE(x == 4);
    REQUIRE(y == -4);
}

TEST_CASE("action binding", "[actions][operators]")
{
    int x = 0;
    auto a = callback([&](int y, int z) { x = y - z; });

    REQUIRE(!(a << empty<int>()).is_ready());

    auto b = a << value(1);
    REQUIRE(b.is_ready());
    REQUIRE(!(b << empty<int>()).is_ready());
    perform_action(b, 4);
    REQUIRE(x == -3);

    auto c = b << 2;
    REQUIRE(c.is_ready());
    perform_action(c);
    REQUIRE(x == -1);

    auto d = a << 3 << 2;
    action<> e = d;
    REQUIRE(e.is_ready());
    perform_action(e);
    REQUIRE(x == 1);

    REQUIRE(!(callback([]() { return false; }, [&](int) {}) << value(0))
                 .is_ready());
}
