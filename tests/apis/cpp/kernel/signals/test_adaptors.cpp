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

TEST_CASE("has_value_view")
{
    auto empty_presence = has_value_view(empty<int>());
    static_assert(view_signal<decltype(empty_presence)>);
    static_assert(!sink_signal<decltype(empty_presence)>);
    CHECK(signal_has_value(empty_presence));
    CHECK(read_signal(empty_presence) == false);

    auto present = has_value_view(value(1));
    CHECK(signal_has_value(present));
    CHECK(read_signal(present) == true);
}

TEST_CASE("ready_to_write_view")
{
    auto unwritable = ready_to_write_view(value(1));
    static_assert(view_signal<decltype(unwritable)>);
    static_assert(!sink_signal<decltype(unwritable)>);
    CHECK(signal_has_value(unwritable));
    CHECK(read_signal(unwritable) == false);

    int x = 1;
    auto writable = ready_to_write_view(ref(x));
    CHECK(signal_has_value(writable));
    CHECK(read_signal(writable) == true);

    auto empty_ready = ready_to_write_view(empty<int>());
    CHECK(read_signal(empty_ready) == false);
}

TEST_CASE("mask a binding")
{
    int x = 1;
    auto d = ref(x);
    auto s = mask(d, true);

    static_assert(std::same_as<decltype(s)::value_type, int>);
    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 1);
    CHECK((s.value_id() == d.value_id()));
    CHECK(signal_ready_to_write(s));
    write_signal(s, 0);
    CHECK(x == 0);

    auto hidden = mask(ref(x), false);
    CHECK_FALSE(signal_has_value(hidden));
    CHECK_FALSE(signal_ready_to_write(hidden));
    CHECK((hidden.value_id() == null_id()));
}

TEST_CASE("mask a read-only signal")
{
    auto d = value(1);
    auto s = mask(d, true);

    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 1);
    CHECK((s.value_id() == d.value_id()));

    auto hidden = mask(value(1), false);
    CHECK_FALSE(signal_has_value(hidden));
    CHECK((hidden.value_id() == null_id()));
}

TEST_CASE("mask a raw value")
{
    int x = 12;
    auto shown = mask(x, true);
    CHECK(signal_has_value(shown));
    CHECK(read_signal(shown) == x);

    auto hidden = mask(x, false);
    CHECK_FALSE(signal_has_value(hidden));
}

TEST_CASE("mask/disable_writes")
{
    int x = 1;
    auto wrapped = ref(x);
    auto unmasked = mask_writes(wrapped, value(true));

    static_assert(view_signal<decltype(unmasked)>);
    static_assert(sink_signal<decltype(unmasked)>);

    CHECK(signal_has_value(unmasked));
    CHECK(read_signal(unmasked) == 1);
    CHECK((unmasked.value_id() == wrapped.value_id()));
    CHECK(signal_ready_to_write(unmasked));
    write_signal(unmasked, 0);
    CHECK(x == 0);

    x = 1;
    auto disabled = disable_writes(ref(x));
    static_assert(sink_signal<decltype(disabled)>);
    CHECK(signal_has_value(disabled));
    CHECK(read_signal(disabled) == 1);
    CHECK_FALSE(signal_ready_to_write(disabled));

    auto empty_flag = mask_writes(ref(x), empty<bool>());
    CHECK(signal_has_value(empty_flag));
    CHECK_FALSE(signal_ready_to_write(empty_flag));

    auto raw_flag = mask_writes(ref(x), false);
    CHECK(signal_has_value(raw_flag));
    CHECK_FALSE(signal_ready_to_write(raw_flag));

    auto read_only = mask_writes(value(1), true);
    static_assert(view_signal<decltype(read_only)>);
    static_assert(!sink_signal<decltype(read_only)>);
    CHECK(signal_has_value(read_only));
    CHECK(read_signal(read_only) == 1);
}

TEST_CASE("mask/disable_reads")
{
    int x = 1;
    auto wrapped = ref(x);
    auto unmasked = mask_reads(wrapped, value(true));

    static_assert(view_signal<decltype(unmasked)>);
    static_assert(sink_signal<decltype(unmasked)>);

    CHECK(signal_has_value(unmasked));
    CHECK(read_signal(unmasked) == 1);
    CHECK((unmasked.value_id() == wrapped.value_id()));
    CHECK(signal_ready_to_write(unmasked));
    write_signal(unmasked, 0);
    CHECK(x == 0);

    x = 1;
    auto disabled = disable_reads(ref(x));
    static_assert(sink_signal<decltype(disabled)>);
    CHECK_FALSE(signal_has_value(disabled));
    CHECK(signal_ready_to_write(disabled));
    write_signal(disabled, 0);
    CHECK(x == 0);

    x = 1;
    auto empty_flag = mask_reads(ref(x), empty<bool>());
    CHECK_FALSE(signal_has_value(empty_flag));
    CHECK(signal_ready_to_write(empty_flag));
    write_signal(empty_flag, 2);
    CHECK(x == 2);

    auto raw_flag = mask_reads(ref(x), false);
    CHECK_FALSE(signal_has_value(raw_flag));
    CHECK(signal_ready_to_write(raw_flag));
}
