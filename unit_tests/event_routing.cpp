#include <alia/data_graph.hpp>
#include <alia/event_routing.hpp>
#include <sstream>

#include <catch.hpp>

using namespace alia;

struct test_event
{
    virtual ~test_event()
    {
    }
};

struct test_context
{
    event_routing_traversal* routing;
    data_traversal* data;
    test_event* event;
};

data_traversal&
get_data_traversal(test_context& ctx)
{
    return *ctx.data;
}

struct ostream_event : test_event
{
    std::ostream* stream;
};

void
do_ostream_text(test_context& ctx, std::string const& text)
{
    ostream_event* oe = dynamic_cast<ostream_event*>(ctx.event);
    if (oe)
    {
        *oe->stream << text << ";";
    }
}

struct find_label_event : test_event
{
    routing_region_ptr region;
    std::string name;
};

void
do_label(test_context& ctx, std::string const& name)
{
    do_ostream_text(ctx, name);

    find_label_event* fle = dynamic_cast<find_label_event*>(ctx.event);
    if (fle)
    {
        if (fle->name == name)
            fle->region = get_active_region(*ctx.routing);
    }
}

struct traversal_function
{
    test_context& ctx;
    int n;
    traversal_function(test_context& ctx, int n) : ctx(ctx), n(n)
    {
    }
    void
    operator()()
    {
        do_ostream_text(ctx, "");

        scoped_routing_region srr(*ctx.routing);
        ALIA_IF(srr.is_relevant())
        {
            do_label(ctx, "root");

            ALIA_IF(n != 0)
            {
                scoped_routing_region srr(*ctx.routing);
                ALIA_IF(srr.is_relevant())
                {
                    do_label(ctx, "nonzero");

                    {
                        scoped_routing_region srr(*ctx.routing);
                        ALIA_IF(srr.is_relevant())
                        {
                            do_label(ctx, "deep");
                        }
                        ALIA_END
                    }
                }
                ALIA_END
            }
            ALIA_END

            ALIA_IF(n & 1)
            {
                scoped_routing_region srr(*ctx.routing);
                ALIA_IF(srr.is_relevant())
                {
                    do_label(ctx, "odd");
                }
                ALIA_END
            }
            ALIA_END
        }
        ALIA_END
    }
};

void
do_traversal(
    data_graph& graph,
    int n,
    test_event& e,
    bool targeted,
    routing_region_ptr const& target = routing_region_ptr())
{
    data_traversal data_traversal;
    scoped_data_traversal sdt(graph, data_traversal);
    event_routing_traversal routing_traversal;

    test_context ctx;
    ctx.event = &e;
    ctx.data = &data_traversal;
    ctx.routing = &routing_traversal;

    traversal_function fn(ctx, n);
    invoke_routed_traversal(
        fn, routing_traversal, data_traversal, targeted, target);
}

void
check_traversal_path(
    int n, std::string const& label, std::string const& expected_path)
{
    data_graph graph;
    routing_region_ptr target;
    {
        find_label_event fle;
        fle.name = label;
        do_traversal(graph, n, fle, false);
        target = fle.region;
    }
    {
        ostream_event oe;
        std::ostringstream s;
        oe.stream = &s;
        do_traversal(graph, n, oe, true, target);
        REQUIRE(s.str() == expected_path);
    }
}

TEST_CASE("targeted_event_dispatch", "[targeted_event_dispatch]")
{
    check_traversal_path(0, "absent", ";");
    check_traversal_path(0, "root", ";root;");
    check_traversal_path(0, "nonzero", ";");
    check_traversal_path(0, "odd", ";");
    check_traversal_path(0, "deep", ";");

    check_traversal_path(1, "absent", ";");
    check_traversal_path(1, "root", ";root;");
    check_traversal_path(1, "nonzero", ";root;nonzero;");
    check_traversal_path(1, "odd", ";root;odd;");
    check_traversal_path(1, "deep", ";root;nonzero;deep;");

    check_traversal_path(2, "absent", ";");
    check_traversal_path(2, "root", ";root;");
    check_traversal_path(2, "nonzero", ";root;nonzero;");
    check_traversal_path(2, "odd", ";");
    check_traversal_path(2, "deep", ";root;nonzero;deep;");
}
