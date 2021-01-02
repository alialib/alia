#include <alia/html/bootstrap.hpp>

#include <alia/html/widgets.hpp>

namespace alia { namespace html { namespace bootstrap {
namespace detail {

element_handle<html::context>
checkbox(html::context ctx, duplex<bool> value, readable<std::string> label)
{
    bool determinate = value.has_value();
    bool checked = determinate && value.read();
    bool disabled = !value.ready_to_write();

    return element(ctx, "div")
        .class_("custom-control", "custom-checkbox")
        .children([&](auto ctx) {
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

html::element_handle<html::context>
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

html::element_handle<html::context>
modal_close_button(html::context ctx)
{
    return close_button(ctx).attr("data-dismiss", "modal");
}

void
close_modal()
{
    emscripten_run_script("jQuery(\"#alia-modal\").modal('hide');");
}

modal_handle
modal(html::context ctx, alia::function_view<void()> content)
{
    modal_data* data;
    if (get_cached_data(ctx, &data))
        create_as_modal_root(data->root.object);

    int asmdom_id;

    invoke_tree(ctx, data->root, [&] {
        auto top_level_modal = element(ctx, "div");
        top_level_modal.class_("modal", "fade")
            .attr("id", "alia-modal")
            .attr("tabindex", "-1")
            .attr("role", "dialog")
            .children([&] {
                div(ctx, "modal-dialog modal-dialog-centered")
                    .attr("role", "document")
                    .children([&] {
                        alia_if(data->active)
                        {
                            div(ctx, "modal-content", content);
                        }
                        alia_end
                    });
            });
        top_level_modal.callback(
            "hidden.bs.modal", [&](auto) { data->active = false; });
        asmdom_id = top_level_modal.asmdom_id();
    });

    on_refresh(ctx, [&](auto ctx) {
        EM_ASM(
            { jQuery(Module['nodes'][$0]).modal('handleUpdate'); }, asmdom_id);
    });

    return modal_handle{*data, asmdom_id};
}

}}} // namespace alia::html::bootstrap
