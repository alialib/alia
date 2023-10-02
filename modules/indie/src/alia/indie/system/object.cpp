#include <alia/indie/system/object.hpp>

#include <alia/indie/layout/specification.hpp>
#include <alia/indie/layout/system.hpp>
#include <alia/indie/layout/traversal.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

void
system::invoke_controller(vanilla_context vanilla_ctx)
{
    indie::traversal traversal;

    layout_style_info style_info;
    initialize_layout_traversal(
        traversal.layout,
        this->layout_state,
        &this->layout_root,
        is_refresh_event(vanilla_ctx),
        &style_info,
        make_vector<float>(200, 200)); // TODO

    if (is_refresh_event(vanilla_ctx))
        traversal.widgets.next_ptr = &this->root_widget;

    auto ctx = extend_context<traversal_tag>(
        extend_context<system_tag>(vanilla_ctx, *this), traversal);

    this->controller(ctx);

    if (is_refresh_event(vanilla_ctx))
        *traversal.widgets.next_ptr = nullptr;
}

}} // namespace alia::indie
