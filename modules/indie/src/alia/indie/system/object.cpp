#include <alia/indie/system/object.hpp>

#include <alia/indie/layout/api.hpp>
#include <alia/indie/layout/system.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

void
system::invoke_controller(vanilla_context vanilla_ctx)
{
    indie::traversal traversal;
    traversal.indie_sys = this;

    scoped_layout_refresh slr;
    scoped_layout_traversal slt;

    geometry_context geometry;
    initialize(
        geometry,
        box<2, double>(
            make_vector<double>(0, 0), vector<2, double>(this->surface_size)));

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
        slt.begin(
            this->layout,
            traversal.layout,
            geometry,
            make_vector<float>(200, 200));
    }

    auto ctx = extend_context<traversal_tag>(vanilla_ctx, traversal);

    this->controller(ctx);
    traversal.widgets.next_ptr = nullptr;
}

}} // namespace alia::indie
