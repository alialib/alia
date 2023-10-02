#include <alia/indie/system/object.hpp>

#include <alia/indie/layout/specification.hpp>
#include <alia/indie/layout/system.hpp>
#include <alia/indie/layout/traversal.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

void
system::invoke_controller(vanilla_context vanilla_ctx)
{
    layout_style_info style_info;

    indie::traversal traversal;
    initialize_layout_traversal(
        traversal.layout,
        &this->root_widget,
        is_refresh_event(vanilla_ctx),
        this->refresh_counter,
        &style_info,
        make_vector<float>(200, 200)); // TODO

    auto ctx = extend_context<traversal_tag>(
        extend_context<system_tag>(vanilla_ctx, *this), traversal);

    this->controller(ctx);
}

}} // namespace alia::indie
