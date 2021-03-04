#include <alia/html/bootstrap/modals.hpp>

#include <alia/html/bootstrap/utilities.hpp>
#include <alia/html/elements.hpp>
#include <alia/html/widgets.hpp>

namespace alia { namespace html { namespace bootstrap {

html::element_handle
internal_modal_handle::close_button()
{
    return bootstrap::close_button(this->context())
        .attr("data-dismiss", "modal");
}

void
internal_modal_handle::close()
{
    EM_ASM({ jQuery(Module['nodes'][$0]).modal('hide'); }, this->asmdom_id());
}

void
modal_handle::activate()
{
    data.active = true;
    EM_ASM({ jQuery(Module['nodes'][$0]).modal('show'); }, this->asmdom_id());
}

modal_handle
modal(
    html::context ctx,
    alia::function_view<void(internal_modal_handle&)> content)
{
    modal_data* data;
    if (get_cached_data(ctx, &data))
        create_as_modal_root(data->root.object);

    scoped_tree_root<element_object> scoped_root;
    if (is_refresh_event(ctx))
        scoped_root.begin(get<html::tree_traversal_tag>(ctx), data->root);

    element_handle modal = element(ctx, "div");
    modal.class_("modal")
        .attr("tabindex", "-1")
        .attr("role", "dialog")
        .content([&] {
            div(ctx, "modal-dialog modal-dialog-centered")
                .attr("role", "document")
                .content([&] {
                    ALIA_IF(data->active)
                    {
                        div(ctx, "modal-content", [&] {
                            internal_modal_handle handle(modal);
                            content(handle);
                        });
                        refresh_handler(ctx, [&](auto ctx) {
                            EM_ASM(
                                {
                                    jQuery(Module['nodes'][$0])
                                        .modal('handleUpdate');
                                },
                                modal.asmdom_id());
                        });
                    }
                    ALIA_END
                });
        });
    modal.init([&](auto&) {
        EM_ASM(
            {
                jQuery(Module['nodes'][$0])
                    .on(
                        "hidden.bs.modal", function(e) {
                            Module['nodes'][$0].dispatchEvent(
                                new CustomEvent("bs.modal.hidden"));
                        });
            },
            modal.asmdom_id());
    });
    modal.handler("bs.modal.hidden", [&](auto) { data->active = false; });

    return modal_handle(modal, *data);
}

}}} // namespace alia::html::bootstrap
