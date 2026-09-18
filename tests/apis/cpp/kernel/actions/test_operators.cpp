#include <alia/kernel/actions/operators.hpp>

#include <alia/kernel/actions/basic.hpp>
#include <alia/kernel/signals/basic.hpp>

#include <alia/test/kernel/effect_fixture.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::test;

TEST_CASE("copy actions")
{
    effect_fixture fx;
    int x = 1;
    CHECK_FALSE((empty<int>() <<= empty<int>()).is_ready());
    CHECK_FALSE((ref(x) <<= empty<int>()).is_ready());
    CHECK_FALSE((empty<int>() <<= ref(x)).is_ready());
    auto a = ref(x) <<= value(2);
    CHECK(a.is_ready());
    CHECK(x == 1);
    post_action(&fx.ctx, a);
    fx.run();
    CHECK(x == 2);
    auto liberal = ref(x) <<= 4;
    CHECK(liberal.is_ready());
    post_action(&fx.ctx, liberal);
    fx.run();
    CHECK(x == 4);
}

#define TEST_COMPOUND_ASSIGNMENT_OPERATOR(op, normal_form)                    \
    {                                                                         \
        int x = 21;                                                           \
        CHECK_FALSE((empty<int>() op empty<int>()).is_ready());               \
        CHECK_FALSE((ref(x) op empty<int>()).is_ready());                     \
        CHECK_FALSE((empty<int>() op ref(x)).is_ready());                     \
        auto a = ref(x) op value(7);                                          \
        CHECK(a.is_ready());                                                  \
        CHECK(x == 21);                                                       \
        post_action(&fx.ctx, a);                                              \
        fx.run();                                                             \
        CHECK(x == (21 normal_form 7));                                       \
        x = 2;                                                                \
        auto liberal = ref(x) op 6;                                           \
        CHECK(liberal.is_ready());                                            \
        post_action(&fx.ctx, liberal);                                        \
        fx.run();                                                             \
        CHECK(x == (2 normal_form 6));                                        \
    }

TEST_CASE("compound assignment action operators")
{
    effect_fixture fx;
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
        CHECK_FALSE((empty<int>() op).is_ready());                            \
        CHECK_FALSE((op empty<int>()).is_ready());                            \
        {                                                                     \
            auto a = ref(x) op;                                               \
            CHECK(a.is_ready());                                              \
            CHECK(x == 21);                                                   \
            post_action(&fx.ctx, a);                                          \
            fx.run();                                                         \
            CHECK(x == (21 normal_form 1));                                   \
        }                                                                     \
        {                                                                     \
            auto a = op ref(x);                                               \
            CHECK(a.is_ready());                                              \
            post_action(&fx.ctx, a);                                          \
            fx.run();                                                         \
            CHECK(x == (21 normal_form 2));                                   \
        }                                                                     \
    }

TEST_CASE("increment/decrement action operators")
{
    effect_fixture fx;
    TEST_BY_ONE_OPERATOR(++, +)
    TEST_BY_ONE_OPERATOR(--, -)
}

#undef TEST_BY_ONE_OPERATOR

TEST_CASE("combined actions")
{
    effect_fixture fx;
    int x = 1, y = 2;
    auto a = empty<int>() <<= empty<int>();
    auto b = ref(x) <<= value(2);
    auto c = ref(y) <<= value(3);
    CHECK_FALSE((a, b).is_ready());
    CHECK((b, c, b).is_ready());
    post_action(&fx.ctx, (b, c));
    fx.run();
    CHECK(x == 2);
    CHECK(y == 3);
}

TEST_CASE("parameterized combined action")
{
    effect_fixture fx;
    int x = 0, y = 0;
    auto a = callback([&](int n) { x += n; });
    auto b = callback([&](int n) { y -= n; });
    post_action(&fx.ctx, (a, b), 4);
    fx.run();
    CHECK(x == 4);
    CHECK(y == -4);
}

TEST_CASE("action binding")
{
    effect_fixture fx;
    int x = 0;
    auto a = callback([&](int y, int z) { x = y - z; });

    CHECK_FALSE((a << empty<int>()).is_ready());

    auto b = a << value(1);
    CHECK(b.is_ready());
    CHECK_FALSE((b << empty<int>()).is_ready());
    post_action(&fx.ctx, b, 4);
    fx.run();
    CHECK(x == -3);

    auto c = b << 2;
    CHECK(c.is_ready());
    post_action(&fx.ctx, c);
    fx.run();
    CHECK(x == -1);

    auto d = a << 3 << 2;
    action<> e = d;
    CHECK(e.is_ready());
    post_action(&fx.ctx, e);
    fx.run();
    CHECK(x == 1);

    CHECK_FALSE((callback([]() { return false; }, [&](int) {}) << value(0))
                    .is_ready());
}
