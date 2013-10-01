#include <alia/data_graph.hpp>
#include <string>
#include <boost/lexical_cast.hpp>

#define BOOST_TEST_MODULE data
#include "test.hpp"

int n_int_constructs = 0, n_int_destructs = 0, n_int_inits = 0;

struct int_object
{
    int_object() { ++n_int_constructs; n = 1; }
    int_object(int n) : n(n) { ++n_int_constructs; ++n_int_inits; }
    ~int_object() { ++n_int_destructs; }
    int n;
};

int n_string_constructs = 0, n_string_destructs = 0, n_string_inits = 0;

struct string_object
{
    string_object() { ++n_string_constructs; }
    ~string_object() { ++n_string_destructs; }
    std::string s;
};

// The following test tests the low-level mechanics of get_data and data
// life-time management.

void do_int(alia::data_traversal& ctx, int n)
{
    int_object* obj;
    if (get_data(ctx, &obj))
    {
        BOOST_CHECK_EQUAL(obj->n, 1);
        obj->n = n;
        ++n_int_inits;
    }
    else
        BOOST_CHECK_EQUAL(obj->n, n);
}

void do_string(alia::data_traversal& ctx, std::string const& s)
{
    string_object* obj;
    if (get_data(ctx, &obj))
    {
        BOOST_CHECK_EQUAL(obj->s, "");
        obj->s = s;
        ++n_string_inits;
    }
    else
        BOOST_CHECK_EQUAL(obj->s, s);
}

void do_traversal(
    alia::data_graph& graph, int n, int a, int b, int c, int d, int backwards,
    int reentrant)
{
    using namespace alia;
    data_traversal ctx;
    scoped_data_traversal sdt(graph, ctx);
    do_int(ctx, 0);
    do_int(ctx, -2);
    alia_if (reentrant)
    {
        do_traversal(graph, n, a, b, c, d, backwards, 0);
    }
    alia_end
    alia_if (b || c || d)
    {
        do_string(ctx, "x");
        alia_if (b)
        {
            do_string(ctx, "y");
            naming_context nc(ctx);
            if (backwards)
            {
                for (int i = n + 3; i >= n; --i)
                {
                    named_block nb(nc, make_id(i));
                    do_int(ctx, i);
                    do_string(ctx, "p");
                }
            }
            else
            {
                for (int i = n; i < n + 4; ++i)
                {
                    named_block nb(nc, make_id(i));
                    do_int(ctx, i);
                    do_string(ctx, "p");
                }
            }
            do_string(ctx, "z");
        }
        alia_end
        alia_if (c)
        {
            alia_for (int i = 0; i < n; ++i)
            {
                do_int(ctx, i);
                do_string(ctx, "q");
            }
            alia_end
        }
        alia_end
        do_int(ctx, 6);
        alia_if (d)
        {
            naming_context nc(ctx);
            do_int(ctx, 2);
            if (backwards)
            {
                for (int i = n + 3; i >= n; --i)
                {
                    named_block nb(nc, make_id(i));
                    do_int(ctx, i - 1);
                }
            }
            else
            {
                for (int i = n; i < n + 4; ++i)
                {
                    named_block nb(nc, make_id(i));
                    do_int(ctx, i - 1);
                }
            }
            do_string(ctx, "a");
        }
        alia_end
        do_int(ctx, 0);
        do_string(ctx, "z");
    }
    alia_else_if (a)
    {
        do_string(ctx, "alia");
        alia_if (reentrant)
        {
            do_traversal(graph, n, a, b, c, d, backwards, 0);
        }
        alia_end
        naming_context nc(ctx);
        if (backwards)
        {
            for (int i = n + 103; i >= n + 100; --i)
            {
                named_block nb(nc, make_id(i), manual_delete(true));
                do_int(ctx, i - 1);
            }
        }
        else
        {
            for (int i = n + 100; i < n + 104; ++i)
            {
                named_block nb(nc, make_id(i), manual_delete(true));
                do_int(ctx, i - 1);
            }
        }
        do_int(ctx, 42);
    }
    alia_else
    {
        do_int(ctx, 0);
        alia_if (n < 0)
        {
            int i = 0;
            alia_while (i-- > n)
            {
                do_int(ctx, i);
            }
            alia_end
        }
        alia_else
        {
            alia_if (reentrant)
            {
                do_traversal(graph, n, a, b, c, d, backwards, 0);
            }
            alia_end
            alia_switch (n)
            {
             alia_case(0):
                do_int(ctx, 0);
                break;
             alia_case(1):
                do_int(ctx, 0);
             alia_case(2):
             alia_case(3):
                do_int(ctx, 2);
                break;
             alia_default:
                do_int(ctx, 3);
            }
            alia_end
        }
        alia_end
    }
    alia_end
    do_int(ctx, -1);
}

#define check_inits(ic, sc) \
    BOOST_CHECK_EQUAL(n_int_inits, ic); \
    BOOST_CHECK_EQUAL(n_string_inits, sc);

BOOST_AUTO_TEST_CASE(low_level_test)
{
    n_int_constructs = 0; n_int_destructs = 0; n_int_inits = 0;
    n_string_constructs = 0; n_string_destructs = 0; n_string_inits = 0;

    {
        alia::data_graph graph;

        int ic = 0, sc = 0;

        // different cases in the switch statement
        do_traversal(graph, 0,  0, 0, 0, 0, 0, 0);
        ic += 5;
        check_inits(ic, sc);
        do_traversal(graph, 3,  0, 0, 0, 0, 0, 0);
        ic += 1;
        check_inits(ic, sc);
        do_traversal(graph, 2,  0, 0, 0, 0, 0, 0);
        check_inits(ic, sc);
        do_traversal(graph, 6,  0, 0, 0, 0, 0, 0);
        ic += 1;
        check_inits(ic, sc);
        do_traversal(graph, 4,  0, 0, 0, 0, 0, 0);
        check_inits(ic, sc);
        do_traversal(graph, 1,  0, 0, 0, 0, 0, 0);
        ic += 1;
        check_inits(ic, sc);
        do_traversal(graph, 0,  0, 0, 0, 0, 0, 0);
        check_inits(ic, sc);

        // different numbers of iterations in the while loop
        do_traversal(graph, -3, 0, 0, 0, 0, 0, 0);
        ic += 3;
        check_inits(ic, sc);
        do_traversal(graph, -2, 0, 0, 0, 0, 0, 0);
        check_inits(ic, sc);
        do_traversal(graph, -6, 0, 0, 0, 0, 0, 0);
        ic += 3;
        check_inits(ic, sc);
        do_traversal(graph, -1, 0, 0, 0, 0, 0, 0);
        check_inits(ic, sc);
        do_traversal(graph, -7, 0, 0, 0, 0, 0, 0);
        ic += 1;
        check_inits(ic, sc);
        do_traversal(graph, -3, 0, 0, 0, 0, 0, 0);
        check_inits(ic, sc);
        do_traversal(graph, -1, 0, 0, 0, 0, 0, 0);
        check_inits(ic, sc);
        // and one more case
        do_traversal(graph, 0,  0, 0, 0, 0, 0, 0);
        check_inits(ic, sc);

        do_traversal(graph, 0,  1, 0, 0, 0, 0, 0);
        ic += 5; sc += 1;
        check_inits(ic, sc);

        // varying numbers of iterations in the for loop
        do_traversal(graph, 0,  0, 0, 1, 0, 0, 0);
        ic += 2; sc += 2;
        check_inits(ic, sc);
        do_traversal(graph, 3,  0, 0, 1, 0, 0, 0);
        ic += 3; sc += 3;
        check_inits(ic, sc);
        do_traversal(graph, 12, 0, 0, 1, 0, 0, 0);
        ic += 9; sc += 9;
        check_inits(ic, sc);
        do_traversal(graph, 6,  0, 0, 1, 0, 0, 0);
        check_inits(ic, sc);
        do_traversal(graph, 12, 0, 0, 1, 0, 0, 0);
        check_inits(ic, sc);

        do_traversal(graph, 0,  0, 1, 1, 1, 0, 0);
        ic += 9; sc += 7;
        check_inits(ic, sc);
        // At this point, all branches have been followed, so all future
        // initializations are from named blocks...

        // Test that named blocks aren't deleted if they're inside unexecuted
        // branches.
        do_traversal(graph, 0,  0, 0, 0, 0, 0, 0);
        check_inits(ic, sc);
        do_traversal(graph, 0,  0, 1, 1, 1, 0, 0);
        check_inits(ic, sc);

        // different sets of named blocks, some in reverse order
        do_traversal(graph, 1,  0, 0, 0, 1, 0, 0);
        ic += 1;
        check_inits(ic, sc);
        do_traversal(graph, 6,  0, 0, 0, 1, 0, 0);
        ic += 4;
        check_inits(ic, sc);
        do_traversal(graph, 7,  0, 0, 0, 1, 1, 0);
        ic += 1;
        check_inits(ic, sc);
        do_traversal(graph, 1,  0, 0, 0, 1, 0, 0);
        ic += 4;
        check_inits(ic, sc);
        do_traversal(graph, 1,  0, 0, 0, 1, 1, 0);
        check_inits(ic, sc);
        do_traversal(graph, 1,  0, 0, 0, 1, 0, 0);
        check_inits(ic, sc);

        // Test that blocks with the MANUAL_DELETE flag aren't deleted if
        // they're not seen.
        do_traversal(graph, 1,  1, 0, 0, 0, 0, 0);
        ic += 1;
        check_inits(ic, sc);
        do_traversal(graph, 0,  1, 0, 0, 0, 0, 0);
        check_inits(ic, sc);
        do_traversal(graph, 6,  1, 0, 0, 0, 0, 0);
        ic += 4;
        check_inits(ic, sc);
        do_traversal(graph, 1,  1, 0, 0, 0, 0, 0);
        do_traversal(graph, 0,  1, 0, 0, 0, 0, 0);
        check_inits(ic, sc);
        do_traversal(graph, 1,  1, 0, 0, 0, 1, 0);
        check_inits(ic, sc);

        // Deleting has no effect if the named block is still being invoked.
        delete_named_block(graph, alia::make_id(101));
        do_traversal(graph, 1,  1, 0, 0, 0, 0, 0);
        check_inits(ic, sc);

        // Now test that manual deletion of blocks actually works.
        delete_named_block(graph, alia::make_id(100));
        do_traversal(graph, 0,  1, 0, 0, 0, 0, 0);
        ic += 1;
        check_inits(ic, sc);
        delete_named_block(graph, alia::make_id(100));
        do_traversal(graph, 6,  1, 0, 0, 0, 0, 0);
        do_traversal(graph, 0,  1, 0, 0, 0, 0, 0);
        ic += 2;
        check_inits(ic, sc);
        delete_named_block(graph, alia::make_id(106));
        delete_named_block(graph, alia::make_id(107));
        do_traversal(graph, 6,  1, 0, 0, 0, 0, 0);
        ic += 2;
        check_inits(ic, sc);

        // Test that recursive traversals don't cause any leaks.
        // (There was a bug where it caused leaks with named blocks.)
        do_traversal(graph, 6,  0, 0, 0, 0, 0, 1);
        do_traversal(graph, 0,  1, 0, 0, 0, 0, 1);
        do_traversal(graph, 1,  0, 0, 0, 0, 0, 1);
    }

    BOOST_CHECK_EQUAL(n_int_inits, n_int_constructs);
    BOOST_CHECK_EQUAL(n_int_inits, n_int_destructs);

    BOOST_CHECK_EQUAL(n_string_inits, n_string_constructs);
    BOOST_CHECK_EQUAL(n_string_inits, n_string_destructs);
}

// The following tests that the low-level mechanics of cached data work.

void do_cached_int(alia::data_traversal& ctx, int n)
{
    int_object* obj;
    if (get_cached_data(ctx, &obj))
    {
        BOOST_CHECK_EQUAL(obj->n, 1);
        obj->n = n;
        ++n_int_inits;
    }
    else
        BOOST_CHECK_EQUAL(obj->n, n);
}

void do_keyed_int(alia::data_traversal& ctx, int n)
{
    alia::keyed_data_accessor<int_object> obj;
    if (get_keyed_data(ctx, alia::make_id(n), &obj))
    {
        BOOST_CHECK(!obj.is_gettable());
        set(obj, int_object(n * 2));
    }
    else
        BOOST_CHECK_EQUAL(get(obj).n, n * 2);
}

void do_traversal(alia::data_graph& graph, int n, int a, int b)
{
    alia::data_traversal ctx;
    alia::scoped_data_traversal sdt(graph, ctx);
    do_cached_int(ctx, 0);
    do_int(ctx, -2);
    alia_if (a)
    {
        alia_for (int i = 0; i < n; ++i)
        {
            do_cached_int(ctx, i);
        }
        alia_end
        do_int(ctx, 0);
    }
    alia_else_if (b)
    {
        alia::naming_context nc(ctx);
        for (int i = n + 103; i >= n + 100; --i)
        {
            alia::named_block nb(nc, alia::make_id(i));
            do_int(ctx, 1 - i);
            do_cached_int(ctx, i - 1);
        }
        do_cached_int(ctx, 1);
    }
    alia_end
    alia_switch (a)
    {
     alia_case(0):
        do_cached_int(ctx, 0);
        break;
     alia_case(1):
        do_cached_int(ctx, 1);
        break;
    }
    alia_end
    do_cached_int(ctx, -1);
    do_keyed_int(ctx, a + b);
}

BOOST_AUTO_TEST_CASE(cached_data_test)
{
    n_int_constructs = 0; n_int_destructs = 0; n_int_inits = 0;
    n_string_constructs = 0; n_string_destructs = 0; n_string_inits = 0;

    {
        alia::data_graph graph;

        int ic = 0, sc = 0;

        do_traversal(graph, 0,  0, 0);
        ic += 5;
        check_inits(ic, sc);
        do_traversal(graph, 3,  1, 0);
        ic += 6;
        check_inits(ic, sc);
        do_traversal(graph, 2,  0, 1);
        ic += 10;
        check_inits(ic, sc);
        do_traversal(graph, 3,  1, 0);
        ic += 4;
        check_inits(ic, sc);
        do_traversal(graph, 2,  0, 1);
        ic += 6;
        check_inits(ic, sc);
    }

    BOOST_CHECK_EQUAL(n_int_constructs, n_int_destructs);

    BOOST_CHECK_EQUAL(n_string_inits, n_string_constructs);
    BOOST_CHECK_EQUAL(n_string_inits, n_string_destructs);
}

// The following tests that get_state (in its various forms) works properly.

void do_int_state(alia::data_traversal& ctx, int n)
{
    alia::state_accessor<int_object> accessor = get_state(ctx, int_object(n));
    BOOST_CHECK_EQUAL(get(accessor).n, n);
}

void do_alt_int_state(alia::data_traversal& ctx, int n)
{
    alia::state_accessor<int_object> accessor;
    if (get_state(ctx, &accessor))
    {
        BOOST_CHECK_EQUAL(get(accessor).n, 1);
        set(select_field(accessor, &int_object::n), n);
    }
    else
        BOOST_CHECK_EQUAL(get(accessor).n, n);
}

void do_state_traversal(alia::data_graph& graph, int n)
{
    alia::data_traversal ctx;
    alia::scoped_data_traversal traversal(graph, ctx);
    do_int_state(ctx, 0);
    alia_if (n > 2)
    {
        do_int_state(ctx, 12);
    }
    alia_end
    do_alt_int_state(ctx, -2);
    alia_for (int i = 0; i < n; ++i)
    {
        do_int_state(ctx, i);
    }
    alia_end
}

BOOST_AUTO_TEST_CASE(state_test)
{
    {
        alia::data_graph graph;

        do_state_traversal(graph, 0);
        do_state_traversal(graph, 7);
        do_state_traversal(graph, 1);
        do_state_traversal(graph, 4);
        do_state_traversal(graph, 0);
    }
}

#if 0

// The following tests that get_computed_data works properly.

static int n_foo_calls;

struct foo_fn
{
    size_t operator()(std::string const& x) const
    {
        ++n_foo_calls;
        return x.length();
    }
    typedef size_t result_type;
};

void do_foo_fn(alia::data_traversal& ctx, std::string const& x)
{
    auto r = get_computed_data(ctx, foo_fn(), x);
    BOOST_CHECK_EQUAL(get(r), x.length());
}

void do_lambda_fn(alia::data_traversal& ctx, int n)
{
    auto r = get_computed_data(ctx, [] (int x) { return x * 2; }, n);
    BOOST_CHECK_EQUAL(get(r), n * 2);
}

void do_computing_traversal(alia::data_graph& graph, int n)
{
    alia::data_traversal ctx;
    alia::scoped_data_traversal sdt(graph, ctx);
    do_foo_fn(ctx, "mercury");
    do_lambda_fn(ctx, 5);
    alia_if (n > 2)
    {
        do_foo_fn(ctx, "venus");
    }
    alia_end
    do_foo_fn(ctx, "earth");
    alia_for (int i = 0; i < n; ++i)
    {
        do_foo_fn(ctx, boost::lexical_cast<std::string>(i));
    }
    alia_end
}

#define check_foo_calls(n) \
    BOOST_CHECK_EQUAL(n_foo_calls, n)

BOOST_AUTO_TEST_CASE(computing_test)
{
    {
        alia::data_graph graph;

        n_foo_calls = 0;

        int nfc = 0;
        do_computing_traversal(graph, 0);
        nfc += 2;
        check_foo_calls(nfc);
        do_computing_traversal(graph, 0);
        check_foo_calls(nfc);
        do_computing_traversal(graph, 1);
        nfc += 1;
        check_foo_calls(nfc);
        do_computing_traversal(graph, 3);
        nfc += 3;
        check_foo_calls(nfc);
        do_computing_traversal(graph, 4);
        nfc += 1;
        check_foo_calls(nfc);
        do_computing_traversal(graph, 0);
        check_foo_calls(nfc);
        do_computing_traversal(graph, 3);
        nfc += 4;
        check_foo_calls(nfc);
    }
}

#endif
