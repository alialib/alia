#include <alia/kernel/signals/adaptors.hpp>

#include <alia/kernel/actions/operators.hpp>
#include <alia/kernel/signals/basic.hpp>

#include <doctest/doctest.h>

using namespace alia;
using namespace alia::operators;

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

inline id_view
make_id_by_reference(movable_object const& v)
{
    return alia::make_id(v.n);
}

} // namespace

TEST_CASE("fake_readability")
{
    int x = 0;
    auto s = fake_readability(ref(x));

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK((s.value_id() == null_id()));
    CHECK_FALSE(signal_has_value(s));
    CHECK(signal_ready_to_write(s));
    write_signal(s, 1);
    CHECK(x == 1);
}

TEST_CASE("fake_writability")
{
    auto s = fake_writability(value(0));

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 0);
    CHECK_FALSE(signal_ready_to_write(s));
}

TEST_CASE("signal value movement")
{
    copy_count = 0;
    movable_object m = 2;
    movable_object n = m;
    CHECK(copy_count == 1);

    copy_count = 0;
    movable_object y;
    movable_object x(4);
    perform_action(ref(y) <<= ref(x));
    CHECK(copy_count == 1);
    CHECK(y.n == 4);

    copy_count = 0;
    perform_action(ref(y) <<= move(ref(x)));
    CHECK(copy_count == 0);
    CHECK(y.n == 4);
}
