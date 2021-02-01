#include <alia/html/bootstrap.hpp>

#include <alia/html/widgets.hpp>

namespace alia { namespace html { namespace bootstrap {
namespace detail {

element_handle
checkbox(html::context ctx, duplex<bool> value, readable<std::string> label)
{
    bool determinate = value.has_value();
    bool checked = determinate && value.read();
    bool disabled = !value.ready_to_write();

    return element(ctx, "div")
        .class_("custom-control", "custom-checkbox")
        .children([&] {
            element(ctx, "input")
                .attr("type", "checkbox")
                .class_("custom-control-input")
                .attr("disabled", disabled)
                .attr("id", "custom-check-1")
                .prop("indeterminate", !determinate)
                .prop("checked", checked)
                .callback("change", [&](emscripten::val e) {
                    write_signal(value, e["target"]["checked"].as<bool>());
                });
            element(ctx, "label")
                .class_("custom-control-label")
                .attr("for", "custom-check-1")
                .text(label);
        });
}

} // namespace detail

// COMMON

html::element_handle
close_button(html::context ctx)
{
    return element(ctx, "button")
        .attr("type", "button")
        .class_("close")
        .attr("aria-label", "Close")
        .children([&] {
            element(ctx, "span").attr("aria-hidden", "true").text(u8"\u00D7");
        });
}

// MODALS

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
    alia::function_view<void(internal_modal_handle)> content)
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
        .children([&] {
            div(ctx, "modal-dialog modal-dialog-centered")
                .attr("role", "document")
                .children([&] {
                    ALIA_IF(data->active)
                    {
                        div(ctx, "modal-content", [&] {
                            content(internal_modal_handle(modal));
                        });
                        on_refresh(ctx, [&](auto ctx) {
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
    modal.on_init([&](auto&) {
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
    modal.callback("bs.modal.hidden", [&](auto) { data->active = false; });

    return modal_handle(modal, *data);
}

}}} // namespace alia::html::bootstrap
