#include <alia/indie/system/object.hpp>

#include <alia/indie/layout/api.hpp>
#include <alia/indie/layout/system.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

void
system::invoke_controller(vanilla_context vanilla_ctx)
{
    indie::traversal traversal;

    scoped_layout_refresh slr;
    scoped_layout_traversal slt;

    if (is_refresh_event(vanilla_ctx))
    {
        traversal.widgets.next_ptr = &this->root_widget;
        slr.begin(
            this->layout,
            traversal.layout,
            make_vector<float>(200, 200)); // TODO
    }
    else
    {
        static alia::geometry_context geo;
        slt.begin(
            this->layout, traversal.layout, geo, make_vector<float>(200, 200));
    }

    auto ctx = extend_context<traversal_tag>(vanilla_ctx, traversal);

    this->controller(ctx);
    traversal.widgets.next_ptr = nullptr;
}

void
initialize(
    indie::system& system, std::function<void(indie::context)> controller)
{
    initialize_core_system<indie::vanilla_context>(system, nullptr);
    system.controller = controller;
}

}} // namespace alia::indie
