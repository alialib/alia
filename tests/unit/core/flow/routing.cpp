#include <alia/core/flow/events.hpp>

#include <sstream>

#include <alia/core/flow/macros.hpp>
#include <alia/core/flow/top_level.hpp>
#include <alia/core/system/internals.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace alia;

namespace {

struct ostream_event
{
    std::ostream* stream;
};

void
do_ostream_text(core_context ctx, std::string const& text)
{
    ostream_event* oe;
    if (detect_event(ctx, &oe))
    {
        *oe->stream << text << ";";
    }
}

struct find_label_event
{
    component_container_ptr container;
    std::string name;
};

void
do_label(core_context ctx, std::string const& name)
{
    do_ostream_text(ctx, name);

    event_handler<find_label_event>(ctx, [name](auto ctx, auto& fle) {
        if (fle.name == name)
            fle.container = get_active_component_container(ctx);
    });
}

struct traversal_function
{
    int n;
    traversal_function(int n) : n(n)
    {
    }
    void
    operator()(core_context ctx)
    {
        do_ostream_text(ctx, "");

        scoped_component_container srr0(ctx);
        ALIA_IF(srr0.is_on_route())
        {
            do_label(ctx, "root");

            ALIA_IF(n != 0)
            {
                scoped_component_container srr1(ctx);
                ALIA_IF(srr1.is_on_route())
                {
                    do_label(ctx, "nonzero");

                    {
                        scoped_component_container srr2(ctx);
                        ALIA_IF(srr2.is_on_route())
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
                scoped_component_container srr(ctx);
                ALIA_IF(srr.is_on_route())
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
    initialize_standalone_system(sys, traversal_function(n));
    component_container_ptr target;
    {
        find_label_event fle;
        fle.name = label;
        detail::dispatch_untargeted_event(sys, fle);
        target = fle.container;
    }
    {
        ostream_event oe;
        std::ostringstream s;
        oe.stream = &s;
        detail::dispatch_targeted_event(sys, oe, target);
        REQUIRE(s.str() == expected_path);
    }
}

} // namespace

TEST_CASE("targeted_event_dispatch", "[flow][routing]")
{
    check_traversal_path(0, "absent", "");
    check_traversal_path(0, "root", ";root;");
    check_traversal_path(0, "nonzero", "");
    check_traversal_path(0, "odd", "");
    check_traversal_path(0, "deep", "");

    check_traversal_path(1, "absent", "");
    check_traversal_path(1, "root", ";root;");
    check_traversal_path(1, "nonzero", ";root;nonzero;");
    check_traversal_path(1, "odd", ";root;odd;");
    check_traversal_path(1, "deep", ";root;nonzero;deep;");

    check_traversal_path(2, "absent", "");
    check_traversal_path(2, "root", ";root;");
    check_traversal_path(2, "nonzero", ";root;nonzero;");
    check_traversal_path(2, "odd", "");
    check_traversal_path(2, "deep", ";root;nonzero;deep;");
}
