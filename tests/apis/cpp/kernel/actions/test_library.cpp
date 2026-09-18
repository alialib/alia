#include <alia/kernel/actions/library.hpp>

#include <alia/kernel/signals/adaptors.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/state.hpp>

#include <alia/test/kernel/effect_fixture.hpp>

#include <doctest/doctest.h>

#include <map>
#include <vector>

using namespace alia;
using namespace alia::test;

namespace {

int copy_count = 0;

struct movable_object
{
    movable_object() : n(-1)
    {
    }
    movable_object(int n) : n(n)
    {
    }
    movable_object(movable_object&& other) noexcept
    {
        n = other.n;
    }
    movable_object(movable_object const& other) noexcept
    {
        n = other.n;
        ++copy_count;
    }
    movable_object&
    operator=(movable_object&& other) noexcept
    {
        n = other.n;
        return *this;
    }
    movable_object&
    operator=(movable_object const& other) noexcept
    {
        n = other.n;
        ++copy_count;
        return *this;
    }
    int n;
};

inline bool
operator==(movable_object a, movable_object b)
{
    return a.n == b.n;
}
inline bool
operator<(movable_object a, movable_object b)
{
    return a.n < b.n;
}

inline id_view
make_id_by_reference(movable_object const& v)
{
    return alia::make_id(v.n);
}

template<class Value>
struct test_state
{
    effect_fixture* fx;
    state_storage<Value> storage;

    test_state(effect_fixture& fixture, Value initial) : fx(&fixture)
    {
        write_signal(&fx->ctx, binding(), std::move(initial));
        fx->run();
    }

    auto
    binding()
    {
        return make_state_binding(storage, &fx->ctx);
    }
};

} // namespace

TEST_CASE("toggle action")
{
    effect_fixture fx;
    bool x = false;
    {
        auto a = actions::toggle(ref(x));
        CHECK(a.is_ready());
        post_action(&fx.ctx, a);
        fx.run();
        CHECK(x);
    }
    {
        auto a = actions::toggle(ref(x));
        CHECK(a.is_ready());
        post_action(&fx.ctx, a);
        fx.run();
        CHECK_FALSE(x);
    }

    {
        auto a = actions::toggle(empty<bool>());
        CHECK_FALSE(a.is_ready());
    }
}

TEST_CASE("push_back action")
{
    effect_fixture fx;
    test_state x{fx, std::vector<int>{1, 2}};
    {
        auto a = actions::push_back(x.binding());
        CHECK(a.is_ready());
        post_action(&fx.ctx, a, 3);
        fx.run();
        CHECK(read_signal(x.binding()) == (std::vector<int>{1, 2, 3}));
    }
    {
        auto a = actions::push_back(x.binding()) << 4;
        CHECK(a.is_ready());
        post_action(&fx.ctx, a);
        fx.run();
        CHECK(read_signal(x.binding()) == (std::vector<int>{1, 2, 3, 4}));
    }

    {
        auto a = actions::push_back(empty<std::vector<int>>());
        CHECK_FALSE(a.is_ready());
    }
}

TEST_CASE("push_back movable")
{
    effect_fixture fx;
    test_state x{
        fx,
        std::vector<movable_object>{movable_object(1), movable_object(2)}};
    {
        auto a = actions::push_back(x.binding()) << value(movable_object(3));
        CHECK(a.is_ready());
        post_action(&fx.ctx, a);
        fx.run();
        CHECK(
            read_signal(x.binding())
            == (std::vector<movable_object>{
                movable_object(1), movable_object(2), movable_object(3)}));
    }
}

TEST_CASE("erase_index action")
{
    effect_fixture fx;
    test_state x{fx, std::vector<int>{1, 2, 3, 4}};
    {
        auto a = actions::erase_index(x.binding(), 2);
        CHECK(a.is_ready());
        post_action(&fx.ctx, a);
        fx.run();
        CHECK(read_signal(x.binding()) == (std::vector<int>{1, 2, 4}));
    }
    {
        auto a = actions::erase_index(x.binding(), value(0));
        CHECK(a.is_ready());
        post_action(&fx.ctx, a);
        fx.run();
        CHECK(read_signal(x.binding()) == (std::vector<int>{2, 4}));
    }

    {
        auto a = actions::erase_index(x.binding(), empty<size_t>());
        CHECK_FALSE(a.is_ready());
    }
    {
        auto a = actions::erase_index(empty<std::vector<int>>(), value(0));
        CHECK_FALSE(a.is_ready());
    }
    {
        test_state unreadwritable{fx, std::vector<int>{1, 2}};
        auto a = actions::erase_index(
            fake_writability(unreadwritable.binding()), value(0));
        CHECK_FALSE(a.is_ready());
    }
}

TEST_CASE("erase_index movement")
{
    effect_fixture fx;
    test_state x{
        fx,
        std::vector<movable_object>{
            movable_object(1), movable_object(2), movable_object(3)}};
    {
        auto a = actions::erase_index(x.binding(), 1);
        CHECK(a.is_ready());
        post_action(&fx.ctx, a);
        fx.run();
        CHECK(
            read_signal(x.binding())
            == (std::vector<movable_object>{
                movable_object(1), movable_object(3)}));
    }
}

TEST_CASE("erase_key action")
{
    effect_fixture fx;
    test_state x{fx, std::map<int, int>{{1, 2}, {2, 4}, {3, 6}}};
    {
        auto a = actions::erase_key(x.binding(), 2);
        CHECK(a.is_ready());
        post_action(&fx.ctx, a);
        fx.run();
        CHECK(read_signal(x.binding()) == (std::map<int, int>{{1, 2}, {3, 6}}));
    }
    {
        auto a = actions::erase_key(x.binding(), value(1));
        CHECK(a.is_ready());
        post_action(&fx.ctx, a);
        fx.run();
        CHECK(read_signal(x.binding()) == (std::map<int, int>{{3, 6}}));
    }
    {
        auto a = actions::erase_key(x.binding(), empty<int>());
        CHECK_FALSE(a.is_ready());
    }
    {
        auto a = actions::erase_key(empty<std::map<int, int>>(), value(0));
        CHECK_FALSE(a.is_ready());
    }
    {
        test_state unreadwritable{fx, std::map<int, int>{{1, 2}}};
        auto a = actions::erase_key(
            fake_writability(unreadwritable.binding()), value(0));
        CHECK_FALSE(a.is_ready());
    }
}

TEST_CASE("erase_key movement")
{
    effect_fixture fx;
    test_state x{
        fx,
        std::map<int, movable_object>{
            {1, movable_object(1)},
            {2, movable_object(2)},
            {3, movable_object(3)}}};
    {
        auto a = actions::erase_key(x.binding(), 1);
        CHECK(a.is_ready());
        post_action(&fx.ctx, a);
        fx.run();
        CHECK(
            read_signal(x.binding())
            == (std::map<int, movable_object>{
                {2, movable_object(2)}, {3, movable_object(3)}}));
    }
}

TEST_CASE("actions::apply")
{
    effect_fixture fx;
    auto add = [](int x, int y) { return x + y; };
    int x = 0;
    auto a = actions::apply(add, ref(x), value(1));
    CHECK(a.is_ready());
    post_action(&fx.ctx, a);
    fx.run();
    CHECK(x == 1);
}
