#ifndef ALIA_TESTING_FLOW_TESTER_HPP
#define ALIA_TESTING_FLOW_TESTER_HPP

#include <alia/context/interface.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/flow/events.hpp>
#include <alia/system/internals.hpp>

#include <sstream>

#include <testing.hpp>

using namespace alia;

// The following define a small framework for testing data traversal mechanics.
// The idea is that we execute various traversals over test data graphs
// (generally multiple times over each graph) and log various events
// (allocating new nodes, destroying old ones, and visiting existing nodes). We
// can then check that the log matches expectations.

extern std::stringstream the_log;

// Clear the log.
// It's best to do this explicitly at the beginning of each test in case the
// previous one failed and left the log in a bad state.
void
clear_log();

// Check that the log contains the expected contents and clear it.
void
check_log(std::string const& expected_contents);

struct int_object
{
    int_object() : n(-1)
    {
    }
    int_object(int n) : n(n)
    {
    }
    int_object(int_object&& other)
    {
        n = other.n;
        moved_out = false;
        other.moved_out = true;
    }
    int_object&
    operator=(int_object&& other)
    {
        n = other.n;
        moved_out = false;
        other.moved_out = true;
        return *this;
    }
    ~int_object()
    {
        if (!moved_out)
            the_log << "destructing int;";
    }
    int n;
    bool moved_out = false;
};

template<class Context>
void
do_int(Context& ctx, int n)
{
    int_object* obj;
    if (get_data(ctx, &obj))
    {
        REQUIRE(obj->n == -1);
        obj->n = n;
        the_log << "initializing int: " << n << ";";
    }
    else
    {
        REQUIRE(obj->n == n);
        the_log << "visiting int: " << n << ";";
    }
}

template<class Context>
void
do_cached_int(Context& ctx, int n)
{
    int_object* obj;
    if (get_cached_data(ctx, &obj))
    {
        REQUIRE(obj->n == -1);
        obj->n = n;
        the_log << "initializing cached int: " << n << ";";
    }
    else
    {
        REQUIRE(obj->n == n);
        the_log << "visiting cached int: " << n << ";";
    }
}

template<class Context>
void
do_keyed_int(Context& ctx, int n)
{
    keyed_data_signal<int_object> obj;
    if (get_keyed_data(ctx, make_id(n), &obj))
    {
        REQUIRE(!obj.has_value());
        write_signal(obj, int_object(n * 2));
        the_log << "initializing keyed int: " << n << ";";
    }
    else
    {
        REQUIRE(read_signal(obj).n == n * 2);
        REQUIRE(obj.value_id() == make_id(n));
        the_log << "visiting keyed int: " << n << ";";
    }
}

template<class Controller>
void
do_traversal(
    data_graph& graph, Controller const& controller, bool with_gc = true)
{
    alia::system sys;
    event_traversal event;

    data_traversal data;
    scoped_data_traversal sdt(graph, data);

    timing_subsystem timing;
    timing.tick_counter = 0;

    context_storage storage;
    context ctx = make_context(&storage, sys, event, data, timing);

    if (!with_gc)
        disable_gc(data);
    controller(ctx);
}

#endif
