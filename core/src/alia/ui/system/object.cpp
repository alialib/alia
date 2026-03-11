#include <alia/ui/system/object.h>

#include <alia/abi/base/geometry.h>
#include <alia/context.h>
#include <alia/impl/events.hpp>

namespace alia {

void
invoke_controller(ui_system& system, context& ctx)
{
    // TODO
    // layout_style_info style_info{
    //     make_layout_vector(4, 4),
    //     16,
    //     make_layout_vector(12, 16),
    //     12,
    //     this->magnification};

    // geometry_context geometry;
    // initialize(
    //     geometry,
    //     make_box(make_vector<double>(0, 0), vector<2,
    //     double>(surface_size)));

    // ui_traversal traversal;

    // initialize_layout_traversal(
    //     this->layout,
    //     traversal.layout,
    //     is_refresh_event(vanilla_ctx),
    //     is_refresh_event(vanilla_ctx) ? nullptr : &geometry,
    //     &style_info,
    //     this->dpi);

    // auto ctx = add_context_object<ui_traversal_tag>(
    //     add_context_object<ui_system_tag>(vanilla_ctx, std::ref(*this)),
    //     std::ref(traversal));

    system.controller(ctx);
}

} // namespace alia
