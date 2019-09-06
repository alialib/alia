#include <alia/events.hpp>
#include <alia/system.hpp>
#include <sstream>

#include <catch.hpp>

using namespace alia;

struct ostream_event
{
    std::ostream* stream;
};

void
do_ostream_text(context& ctx, std::string const& text)
{
    ostream_event* oe;
    if (detect_event(ctx, &oe))
    {
        *oe->stream << text << ";";
    }
}

struct find_label_event
{
    routing_region_ptr region;
    std::string name;
};

void
do_label(context& ctx, std::string const& name)
{
    do_ostream_text(ctx, name);

    find_label_event* fle;
    if (detect_event(ctx, &fle))
    {
        if (fle->name == name)
            fle->region = get_active_routing_region(ctx);
    }
}

struct traversal_function
{
    int n;
    traversal_function(int n) : n(n)
    {
    }
    void
    operator()(context ctx)
    {
        do_ostream_text(ctx, "");

        REQUIRE(!get_active_routing_region(ctx));

        scoped_routing_region srr(ctx);
        ALIA_IF(srr.is_relevant())
        {
            do_label(ctx, "root");

            ALIA_IF(n != 0)
            {
                scoped_routing_region srr(ctx);
                ALIA_IF(srr.is_relevant())
                {
                    do_label(ctx, "nonzero");

                    {
                        scoped_routing_region srr(ctx);
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
                scoped_routing_region srr(ctx);
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
check_traversal_path(
    int n, std::string const& label, std::string const& expected_path)
{
    alia::system sys;
    sys.controller = traversal_function(n);
    routing_region_ptr target;
    {
        find_label_event fle;
        fle.name = label;
        dispatch_event(sys, fle);
        target = fle.region;
    }
    {
        ostream_event oe;
        std::ostringstream s;
        oe.stream = &s;
        dispatch_targeted_event(sys, oe, target);
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
