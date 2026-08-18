#include <alia/kernel/signals/adaptors.hpp>

#include <alia/kernel/actions/operators.hpp>
#include <alia/kernel/signals/basic.hpp>
#include <alia/kernel/signals/numeric.hpp>
#include <alia/kernel/signals/operators.hpp>

#include <doctest/doctest.h>

#include <optional>

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

template<class Wrapped>
struct transparent_casting_wrapper : casting_signal_wrapper<
                                         transparent_casting_wrapper<Wrapped>,
                                         Wrapped,
                                         typename Wrapped::value_type>
{
    transparent_casting_wrapper(Wrapped wrapped)
        : transparent_casting_wrapper::casting_signal_wrapper(
            std::move(wrapped))
    {
    }
    typename Wrapped::value_type const&
    read() const override
    {
        return this->wrapped_.read();
    }
    typename Wrapped::value_type
    move_out() const override
    {
        return this->wrapped_.move_out();
    }
    typename Wrapped::value_type&
    destructive_ref() const override
    {
        return this->wrapped_.destructive_ref();
    }
    id_view
    write(typename Wrapped::value_type value) const override
    {
        return this->wrapped_.write(std::move(value));
    }
};

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

TEST_CASE("casting_signal_wrapper")
{
    int x = 1;
    auto wrapped = ref(x);
    auto s = transparent_casting_wrapper(wrapped);

    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK((s.value_id() == wrapped.value_id()));
    CHECK(read_signal(s) == 1);
    CHECK(signal_ready_to_write(s));
    write_signal(s, 2);
    CHECK(x == 2);
}

TEST_CASE("signal_cast")
{
    int x = 1;
    auto s = signal_cast<double>(ref(x));

    static_assert(std::same_as<decltype(s)::value_type, double>);
    static_assert(view_signal<decltype(s)>);
    static_assert(sink_signal<decltype(s)>);

    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 1.0);
    CHECK(signal_ready_to_write(s));
    write_signal(s, 0.0);
    CHECK(x == 0);

    auto same = signal_cast<int>(ref(x));
    static_assert(std::same_as<decltype(same), decltype(ref(x))>);
    CHECK(read_signal(same) == 0);
}

TEST_CASE("add_default")
{
    {
        auto s = add_default(value(0), value(1));
        static_assert(view_signal<decltype(s)>);
        static_assert(!sink_signal<decltype(s)>);
        static_assert(
            signal_with<decltype(s), view_caps<signal_move_activated>>);
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 0);
        CHECK(move_from_signal(s) == 0);
    }

    {
        int p = 1;
        auto s = add_default(ref(p), value(0));
        static_assert(view_signal<decltype(s)>);
        static_assert(sink_signal<decltype(s)>);
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 1);
        CHECK(signal_ready_to_write(s));
        write_signal(s, 2);
        CHECK(p == 2);
    }

    {
        int p = 1;
        auto s = add_default(fake_readability(ref(p)), value(0));
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 0);
        CHECK(signal_ready_to_write(s));
        write_signal(s, 2);
        CHECK(p == 2);
    }

    {
        int p = 1;
        auto s = add_default(fake_readability(ref(p)), empty<int>());
        CHECK_FALSE(signal_has_value(s));
        CHECK(signal_ready_to_write(s));
        write_signal(s, 2);
        CHECK(p == 2);
    }

    {
        int p = 1;
        auto s = add_default(disable_writes(ref(p)), empty<int>());
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 1);
        CHECK_FALSE(signal_ready_to_write(s));
    }

    {
        // The overall signal produces a different value ID when using the
        // primary vs the default, even when the two component signals have
        // the same value.
        auto s = add_default(value(0), empty<int>());
        auto t = add_default(empty<int>(), value(0));
        CHECK(signal_has_value(s));
        CHECK(signal_has_value(t));
        CHECK(read_signal(s) == read_signal(t));
        CHECK((s.value_id() != t.value_id()));
    }

    {
        auto s = add_default(empty<int>(), 4);
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 4);
    }
}

TEST_CASE("simplify_id")
{
    {
        auto raw_a = value(1) + value(4);
        auto raw_b = value(2) + value(3);
        CHECK(read_signal(raw_a) == read_signal(raw_b));
        CHECK((raw_a.value_id() != raw_b.value_id()));

        auto a = simplify_id(raw_a);
        auto b = simplify_id(raw_b);
        static_assert(view_signal<decltype(a)>);
        static_assert(!sink_signal<decltype(a)>);
        CHECK(read_signal(a) == 5);
        CHECK(read_signal(b) == 5);
        CHECK((a.value_id() == b.value_id()));
        CHECK((a.value_id() == make_id_by_reference(read_signal(a))));
        CHECK((a.value_id() != raw_a.value_id()));
    }

    {
        double x = 1;
        auto scaled = scale(ref(x), 0.5);
        auto s = simplify_id(scaled);
        static_assert(view_signal<decltype(s)>);
        static_assert(sink_signal<decltype(s)>);
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 0.5);
        CHECK((s.value_id() != scaled.value_id()));
        CHECK(signal_ready_to_write(s));
        write_signal(s, 2);
        CHECK(x == 4);
    }

    {
        auto s = simplify_id(empty<int>());
        CHECK_FALSE(signal_has_value(s));
        CHECK((s.value_id() == null_id()));
    }
}

TEST_CASE("override_id")
{
    {
        auto raw_a = value(1) + value(4);
        auto raw_b = value(2) + value(3);
        CHECK((raw_a.value_id() != raw_b.value_id()));

        auto a = override_id(raw_a, [] { return unit_id(); });
        auto b = override_id(raw_b, [] { return unit_id(); });
        static_assert(view_signal<decltype(a)>);
        static_assert(!sink_signal<decltype(a)>);
        CHECK(read_signal(a) == 5);
        CHECK((a.value_id() == unit_id()));
        CHECK((a.value_id() == b.value_id()));
        CHECK((a.value_id() != raw_a.value_id()));
    }

    {
        int x = 1;
        uint32_t version = 0;
        auto s = override_id(ref(x), [&version] { return make_id(version); });
        static_assert(view_signal<decltype(s)>);
        static_assert(sink_signal<decltype(s)>);
        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 1);
        CHECK((s.value_id() == make_id(uint32_t{0})));

        write_signal(s, 2);
        CHECK(x == 2);
        CHECK((s.value_id() == make_id(uint32_t{0})));
        version = 1;
        CHECK((s.value_id() == make_id(uint32_t{1})));
    }

    {
        auto s = override_id(empty<int>(), [] { return unit_id(); });
        CHECK_FALSE(signal_has_value(s));
        CHECK((s.value_id() == unit_id()));
    }
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

TEST_CASE("unwrap a binding")
{
    {
        auto x = std::optional<int>(1);
        auto d = alia::ref(x);
        auto s = unwrap(d);

        static_assert(std::same_as<decltype(s)::value_type, int>);
        static_assert(view_signal<decltype(s)>);
        static_assert(sink_signal<decltype(s)>);
        static_assert(signal_with<decltype(s), sink_caps<signal_clearable>>);

        CHECK(signal_has_value(s));
        CHECK(read_signal(s) == 1);
        CHECK((s.value_id() == d.value_id()));
        CHECK(signal_ready_to_write(s));
        write_signal(s, 0);
        CHECK(x.has_value());
        CHECK(*x == 0);
    }
    {
        auto x = std::optional<int>();
        auto s = unwrap(alia::ref(x));
        CHECK_FALSE(signal_has_value(s));
        CHECK((s.value_id() == null_id()));
        CHECK(signal_ready_to_write(s));
        write_signal(s, 0);
        CHECK(x.has_value());
        CHECK(*x == 0);
    }
    {
        auto x = std::optional<int>(1);
        auto s = unwrap(alia::ref(x));
        clear_signal(s);
        CHECK_FALSE(x.has_value());
    }
}

TEST_CASE("unwrap a read-only signal")
{
    auto s = unwrap(value(std::optional<int>(3)));
    static_assert(view_signal<decltype(s)>);
    static_assert(!sink_signal<decltype(s)>);
    CHECK(signal_has_value(s));
    CHECK(read_signal(s) == 3);

    auto empty_optional = unwrap(value(std::optional<int>()));
    CHECK_FALSE(signal_has_value(empty_optional));
    CHECK((empty_optional.value_id() == null_id()));
}

TEST_CASE("radio signal")
{
    {
        int selected = 0;
        auto radio = make_radio_signal(ref(selected), value(1));
        static_assert(view_signal<decltype(radio)>);
        static_assert(sink_signal<decltype(radio)>);
        CHECK(signal_has_value(radio));
        CHECK_FALSE(read_signal(radio));
        CHECK(signal_ready_to_write(radio));
        write_signal(radio, true);
        CHECK(selected == 1);
    }
    {
        int selected = 1;
        auto radio = make_radio_signal(ref(selected), value(1));
        CHECK(signal_has_value(radio));
        CHECK(read_signal(radio));
        CHECK(signal_ready_to_write(radio));
    }
}
