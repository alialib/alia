#include <alia/core/flow/components.hpp>

#include <flow/testing.hpp>

TEST_CASE("animating components", "[flow][components]")
{
    clear_log();

    int iteration = -1;

    component_container_ptr top(new component_container);
    component_container_ptr left(new component_container);
    component_container_ptr bottom(new component_container);
    component_container_ptr right(new component_container);

    alia::system sys;
    initialize_standalone_system(sys, [&](core_context ctx) {
        scoped_component_container scoped_top(ctx, &top);
        if (iteration == 0)
            mark_animating_component(ctx);
        {
            scoped_component_container scoped_left(ctx, &left);
            if (iteration == 1)
                mark_animating_component(ctx);
            {
                scoped_component_container scoped_bottom(ctx, &bottom);
                if (iteration == 2)
                    mark_animating_component(ctx);
            }
        }
        {
            scoped_component_container scoped_right(ctx, &right);
            if (iteration == 3)
                mark_animating_component(ctx);
        }
    });

    refresh_system(sys);
    REQUIRE(!top->animating);
    REQUIRE(!left->animating);
    REQUIRE(!bottom->animating);
    REQUIRE(!right->animating);

    ++iteration;
    refresh_system(sys);
    REQUIRE(top->animating);
    REQUIRE(!left->animating);
    REQUIRE(!bottom->animating);
    REQUIRE(!right->animating);

    ++iteration;
    refresh_system(sys);
    REQUIRE(top->animating);
    REQUIRE(left->animating);
    REQUIRE(!bottom->animating);
    REQUIRE(!right->animating);

    ++iteration;
    refresh_system(sys);
    REQUIRE(top->animating);
    REQUIRE(left->animating);
    REQUIRE(bottom->animating);
    REQUIRE(!right->animating);

    ++iteration;
    refresh_system(sys);
    REQUIRE(top->animating);
    REQUIRE(!left->animating);
    REQUIRE(!bottom->animating);
    REQUIRE(right->animating);
}
