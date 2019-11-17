#ifndef ALIA_TESTING_FLOW_TESTER_HPP
#define ALIA_TESTING_FLOW_TESTER_HPP

#include <alia/flow/data_graph.hpp>

#include <sstream>

#include <catch.hpp>

using namespace alia;

// The following define a small framework for testing data traversal mechanics.
// The idea is that we execute various traversals over test data graphs
// (generally multiple times over each graph) and log various events (allocating
// new nodes, destroying old ones, and visiting existing nodes). We can then
// check that the log matches expectations.

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
    ~int_object()
    {
        the_log << "destructing int;";
    }
    int n;
};

void
do_int(context ctx, int n);

void
do_cached_int(context ctx, int n);

void
do_keyed_int(context ctx, int n);

template<class Controller>
void
do_traversal(
    data_graph& graph, Controller const& controller, bool with_gc = true)
{
    data_traversal data;
    scoped_data_traversal sdt(graph, data);

    component_storage storage;
    add_component<data_traversal_tag>(storage, &data);

    context ctx(&storage);

    if (!with_gc)
        disable_gc(ctx);
    controller(ctx);
}

#endif
