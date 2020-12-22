#include <alia/html/bootstrap.hpp>

#include <alia/html/widgets.hpp>

namespace alia {
namespace html {
namespace bootstrap {
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
} // namespace bootstrap
} // namespace html
} // namespace alia
