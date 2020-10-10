#define ALIA_LOWERCASE_MACROS

#include <alia/signals/application.hpp>

#include <testing.hpp>

#include <alia/flow/try_catch.hpp>
#include <alia/signals/basic.hpp>

#include <flow/testing.hpp>
#include <traversal.hpp>

using namespace alia;

TEST_CASE("lazy_apply", "[signals][application]")
{
    auto s1 = lazy_apply([](int i) { return 2 * i; }, value(1));

    typedef decltype(s1) signal_t1;
    REQUIRE(signal_is_readable<signal_t1>::value);
    REQUIRE(!signal_is_writable<signal_t1>::value);

    REQUIRE(signal_has_value(s1));
    REQUIRE(read_signal(s1) == 2);

    auto s2
        = lazy_apply([](int i, int j) { return i + j; }, value(1), value(6));

    typedef decltype(s2) signal_t2;
    REQUIRE(signal_is_readable<signal_t2>::value);
    REQUIRE(!signal_is_writable<signal_t2>::value);

    REQUIRE(signal_has_value(s2));
    REQUIRE(read_signal(s2) == 7);
    REQUIRE(s1.value_id() != s2.value_id());

    // Create some similar signals to make sure that they produce different
    // value IDs.
    auto s3
        = lazy_apply([](int i, int j) { return i + j; }, value(2), value(6));
    auto s4
        = lazy_apply([](int i, int j) { return i + j; }, value(1), value(0));
    REQUIRE(s2.value_id() != s3.value_id());
    REQUIRE(s2.value_id() != s4.value_id());
    REQUIRE(s3.value_id() != s4.value_id());
}

TEST_CASE("duplex_lazy_apply", "[signals][application]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int n = 0;

    captured_id signal_id;

    auto make_controller = [&](double new_value) {
        return [=, &n, &signal_id](context ctx) {
            auto f = [](int x) -> double { return x * 2.0; };
            auto r = [](double x) -> int { return int(x / 2.0 + 0.5); };

            auto s = lazy_duplex_apply(f, r, direct(n));

            typedef decltype(s) signal_t;
            REQUIRE(signal_is_readable<signal_t>::value);
            REQUIRE(signal_is_writable<signal_t>::value);

            REQUIRE(signal_has_value(s));
            REQUIRE(read_signal(s) == n * 2.0);

            signal_id.capture(s.value_id());

            REQUIRE(s.ready_to_write());
            if (new_value > 0)
                write_signal(s, new_value);
        };
    };

    do_traversal(sys, make_controller(0));
    REQUIRE(n == 0);

    {
        captured_id last_id = signal_id;
        do_traversal(sys, make_controller(4));
        REQUIRE(n == 2);

        do_traversal(sys, make_controller(4));
        REQUIRE(n == 2);
        REQUIRE(last_id != signal_id);
    }

    do_traversal(sys, make_controller(0));
    REQUIRE(n == 2);

    do_traversal(sys, make_controller(2));
    REQUIRE(n == 1);

    do_traversal(sys, make_controller(0));
    REQUIRE(n == 1);
}

TEST_CASE("failing lazy_apply", "[signals][application]")
{
    clear_log();

    tree_node<test_object> root;
    root.object.name = "root";

    auto f = [&](size_t i) { return std::string("abcdef").substr(i); };

    int n = 0;

    auto controller = [&](test_context ctx) {
        alia_try
        {
            do_object(ctx, lazy_apply(f, value(n)));
        }
        alia_catch(...)
        {
            do_object(ctx, "error");
        }
        alia_end
    };

    alia::system sys;
    initialize_system(sys, [&](context vanilla_ctx) {
        tree_traversal<test_object> traversal;
        auto ctx = detail::add_context_object<tree_traversal_tag>(
            vanilla_ctx, traversal);
        if (is_refresh_event(ctx))
        {
            traverse_object_tree(traversal, root, [&]() { controller(ctx); });
        }
        else
        {
            controller(ctx);
        }
    });

    n = 0;
    refresh_system(sys);
    REQUIRE(root.object.to_string() == "root(abcdef();)");

    n = 3;
    refresh_system(sys);
    REQUIRE(root.object.to_string() == "root(def();)");

    n = 7;
    refresh_system(sys);
    REQUIRE(root.object.to_string() == "root(error();)");

    n = 7;
    refresh_system(sys);
    REQUIRE(root.object.to_string() == "root(error();)");

    n = 2;
    refresh_system(sys);
    REQUIRE(root.object.to_string() == "root(cdef();)");

    n = 17;
    refresh_system(sys);
    REQUIRE(root.object.to_string() == "root(error();)");
}

TEST_CASE("lazy_lift", "[signals][application]")
{
    auto s = lazy_lift([](int i) { return 2 * i; })(value(1));

    typedef decltype(s) signal_t;
    REQUIRE(signal_is_readable<signal_t>::value);
    REQUIRE(!signal_is_writable<signal_t>::value);

    REQUIRE(signal_has_value(s));
    REQUIRE(read_signal(s) == 2);
}

TEST_CASE("simple apply", "[signals][application]")
{
    int f_call_count = 0;
    auto f = [&](int x, int y) {
        ++f_call_count;
        return x * 2 + y;
    };

    captured_id signal_id;

    alia::system sys;
    initialize_system(sys, [](context) {});

    auto make_controller = [&](int x, int y) {
        return [=, &signal_id](context ctx) {
            auto s = apply(ctx, f, value(x), value(y));

            typedef decltype(s) signal_t;
            REQUIRE(signal_is_readable<signal_t>::value);
            REQUIRE(!signal_is_writable<signal_t>::value);

            REQUIRE(signal_has_value(s));
            REQUIRE(read_signal(s) == x * 2 + y);

            signal_id.capture(s.value_id());
        };
    };

    do_traversal(sys, make_controller(1, 2));
    REQUIRE(f_call_count == 1);
    captured_id last_id = signal_id;

    do_traversal(sys, make_controller(1, 2));
    REQUIRE(f_call_count == 1);
    REQUIRE(last_id == signal_id);
    last_id = signal_id;

    do_traversal(sys, make_controller(2, 2));
    REQUIRE(f_call_count == 2);
    REQUIRE(last_id != signal_id);
    last_id = signal_id;

    do_traversal(sys, make_controller(2, 2));
    REQUIRE(f_call_count == 2);
    REQUIRE(last_id == signal_id);
    last_id = signal_id;

    do_traversal(sys, make_controller(2, 3));
    REQUIRE(f_call_count == 3);
    REQUIRE(last_id != signal_id);
}

TEST_CASE("unready apply", "[signals][application]")
{
    int f_call_count = 0;
    auto f = [&](int x, int y) {
        ++f_call_count;
        return x * 2 + y;
    };

    {
        alia::system sys;
        initialize_system(sys, [](context) {});

        auto make_controller = [=](auto x, auto y) {
            return [=](context ctx) {
                auto s = apply(ctx, f, x, y);

                typedef decltype(s) signal_t;
                REQUIRE(signal_is_readable<signal_t>::value);
                REQUIRE(!signal_is_writable<signal_t>::value);

                REQUIRE(!signal_has_value(s));
            };
        };

        do_traversal(sys, make_controller(empty<int>(), value(2)));
        REQUIRE(f_call_count == 0);

        do_traversal(sys, make_controller(value(1), empty<int>()));
        REQUIRE(f_call_count == 0);
    }
}

TEST_CASE("failing apply", "[signals][application]")
{
    int f_call_count = 0;
    auto f = [&](size_t i) {
        ++f_call_count;
        return std::string("abcdef").substr(i);
    };

    alia::system sys;
    initialize_system(sys, [](context) {});

    auto make_controller = [&](size_t i) {
        return [=](context ctx) {
            alia_try
            {
                do_text(ctx, apply(ctx, f, value(i)));
            }
            alia_catch(...)
            {
                do_text(ctx, value("(error)"));
            }
            alia_end
        };
    };

    check_traversal(sys, make_controller(0), "abcdef;");
    REQUIRE(f_call_count == 1);

    check_traversal(sys, make_controller(0), "abcdef;");
    REQUIRE(f_call_count == 1);

    check_traversal(sys, make_controller(3), "def;");
    REQUIRE(f_call_count == 2);

    check_traversal(sys, make_controller(7), "(error);");
    REQUIRE(f_call_count == 3);

    check_traversal(sys, make_controller(7), "(error);");
    REQUIRE(f_call_count == 3);

    check_traversal(sys, make_controller(3), "def;");
    REQUIRE(f_call_count == 4);

    check_traversal(sys, make_controller(17), "(error);");
    REQUIRE(f_call_count == 5);
}

TEST_CASE("lift", "[signals][application]")
{
    int f_call_count = 0;
    auto f = [&](int x) {
        ++f_call_count;
        return x + 1;
    };

    {
        alia::system sys;
        initialize_system(sys, [](context) {});

        auto controller = [=](context ctx) {
            auto f_lifted = lift(f);
            auto s = f_lifted(ctx, value(0));

            typedef decltype(s) signal_t;
            REQUIRE(signal_is_readable<signal_t>::value);
            REQUIRE(!signal_is_writable<signal_t>::value);

            REQUIRE(signal_has_value(s));
            REQUIRE(read_signal(s) == 1);
        };

        do_traversal(sys, controller);
        REQUIRE(f_call_count == 1);
    }
}

TEST_CASE("duplex_apply", "[signals][application]")
{
    alia::system sys;
    initialize_system(sys, [](context) {});

    int n = 0;

    captured_id signal_id;

    auto make_controller = [&](double new_value) {
        return [=, &n, &signal_id](context ctx) {
            auto f = [](int x) -> double { return x * 2.0; };
            auto r = [](double x) -> int { return int(x / 2.0 + 0.5); };

            auto s = duplex_apply(ctx, f, r, direct(n));

            typedef decltype(s) signal_t;
            REQUIRE(signal_is_readable<signal_t>::value);
            REQUIRE(signal_is_writable<signal_t>::value);

            REQUIRE(signal_has_value(s));
            REQUIRE(read_signal(s) == n * 2.0);

            signal_id.capture(s.value_id());

            REQUIRE(s.ready_to_write());
            if (new_value > 0)
                write_signal(s, new_value);
        };
    };

    do_traversal(sys, make_controller(0));
    REQUIRE(n == 0);

    {
        captured_id last_id = signal_id;
        do_traversal(sys, make_controller(4));
        REQUIRE(n == 2);

        do_traversal(sys, make_controller(4));
        REQUIRE(n == 2);
        REQUIRE(last_id != signal_id);
    }

    do_traversal(sys, make_controller(0));
    REQUIRE(n == 2);

    do_traversal(sys, make_controller(2));
    REQUIRE(n == 1);

    do_traversal(sys, make_controller(0));
    REQUIRE(n == 1);
}

TEST_CASE("alia_mem_fn", "[signals][application]")
{
    auto v = value("test text");
    REQUIRE(read_signal(lazy_apply(ALIA_MEM_FN(length), v)) == 9);
    REQUIRE(
        read_signal(lazy_apply(alia_mem_fn(substr), v, value(5))) == "text");
}
