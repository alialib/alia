#include <alia/indie/system/object.hpp>

#include <alia/indie/layout/api.hpp>
#include <alia/indie/layout/system.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

struct controller_wrapper
{
    indie::system& system;

    void
    operator()(indie::vanilla_context vanilla_ctx)
    {
        indie::traversal traversal;

        scoped_layout_refresh slr;
        scoped_layout_traversal slt;

        if (is_refresh_event(vanilla_ctx))
        {
            traversal.widgets.next_ptr = &system.root_widget;
            slr.begin(
                system.layout,
                traversal.layout,
                make_vector<float>(200, 200)); // TODO
        }
        else
        {
            static alia::geometry_context geo;
            slt.begin(
                system.layout,
                traversal.layout,
                geo,
                make_vector<float>(200, 200));
        }

        auto ctx = extend_context<traversal_tag>(vanilla_ctx, traversal);

        system.controller(ctx);
        traversal.widgets.next_ptr = nullptr;
    }
};

void
initialize(
    indie::system& system, std::function<void(indie::context)> controller)
{
    // Initialize the alia::system and hook it up to the indie::system.
    initialize_system<indie::vanilla_context>(
        system, controller_wrapper{system}, nullptr);
    system.controller = std::move(controller);
}

}} // namespace alia::indie
