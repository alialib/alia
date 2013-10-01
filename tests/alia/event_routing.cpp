#include <alia/event_routing.hpp>
#include <alia/data_graph.hpp>
#include <sstream>

#define BOOST_TEST_MODULE event_dispatch
#include "test.hpp"

struct event
{
    virtual ~event() {}
};

struct context
{
    alia::event_routing_traversal* routing;
    alia::data_traversal* data;
    ::event* event;
};

alia::data_traversal& get_data_traversal(context& ctx)
{ return *ctx.data; }

struct ostream_event : event
{
    std::ostream* stream;
};

void do_ostream_text(context& ctx, std::string const& text)
{
    ostream_event* oe;
    if (oe = dynamic_cast<ostream_event*>(ctx.event))
    {
        *oe->stream << text << ";";
    }
}

struct find_label_event : event
{
    alia::routing_region_ptr region;
    std::string name;
};

void do_label(context& ctx, std::string const& name)
{
    do_ostream_text(ctx, name);

    find_label_event* fle;
    if (fle = dynamic_cast<find_label_event*>(ctx.event))
    {
        if (fle->name == name)
            fle->region = get_active_region(*ctx.routing);
    }
}

struct traversal_function
{
    context& ctx;
    int n;
    traversal_function(context& ctx, int n) : ctx(ctx), n(n) {}
    void operator()()
    {
        do_ostream_text(ctx, "");

        alia::scoped_routing_region srr(*ctx.routing);
        alia_if (srr.is_relevant())
        {
            do_label(ctx, "root");

            alia_if (n != 0)
            {
                alia::scoped_routing_region srr(*ctx.routing);
                alia_if (srr.is_relevant())
                {
                    do_label(ctx, "nonzero");

                    {
                        alia::scoped_routing_region srr(*ctx.routing);
                        alia_if (srr.is_relevant())
                        {
                            do_label(ctx, "deep");
                        }
                        alia_end
                    }
                }
                alia_end
            }
            alia_end

            alia_if (n & 1)
            {
                alia::scoped_routing_region srr(*ctx.routing);
                alia_if (srr.is_relevant())
                {
                    do_label(ctx, "odd");
                }
                alia_end
            }
            alia_end
        }
        alia_end
    }
};

void do_traversal(alia::data_graph& graph, int n, event& e, bool targeted,
    alia::routing_region_ptr const& target = alia::routing_region_ptr())
{
    alia::data_traversal data_traversal;
    alia::scoped_data_traversal sdt(graph, data_traversal);
    alia::event_routing_traversal routing_traversal;

    context ctx;
    ctx.event = &e;
    ctx.data = &data_traversal;
    ctx.routing = &routing_traversal;

    traversal_function fn(ctx, n);
    alia::invoke_routed_traversal(fn, routing_traversal, data_traversal,
        targeted, target);
}

void check_traversal_path(
    int n, std::string const& label,
    std::string const& expected_path)
{
    alia::data_graph graph;
    alia::routing_region_ptr target;
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
        BOOST_CHECK_EQUAL(s.str(), expected_path);
    }
}

BOOST_AUTO_TEST_CASE(targeted_event_dispatch)
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
