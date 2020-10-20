#include <alia/html/widgets.hpp>

namespace alia {
namespace html {

namespace detail {

struct input_data
{
    captured_id value_id;
    string value;
    signal_validation_data validation;
    unsigned version = 0;
};

element_handle<html::context>
input(html::context ctx, duplex<string> value_)
{
    input_data* data;
    get_cached_data(ctx, &data);

    auto value = enforce_validity(ctx, value_, data->validation);

    on_refresh(ctx, [&](auto ctx) {
        if (!value.is_invalidated())
        {
            refresh_signal_shadow(
                data->value_id,
                value,
                [&](string new_value) {
                    data->value = std::move(new_value);
                    ++data->version;
                },
                [&]() {
                    data->value.clear();
                    ++data->version;
                });
        }
    });

    return element(ctx, "input")
        .attr(
            "class",
            conditional(
                value.is_invalidated(), "invalid-input", "form-control"))
        .prop("value", data->value)
        .callback("input", [=](emscripten::val& e) {
            auto new_value = e["target"]["value"].as<std::string>();
            write_signal(value, new_value);
            data->value = new_value;
            ++data->version;
        });
}

void
button(html::context ctx, readable<std::string> text, action<> on_click)
{
    element(ctx, "button")
        .attr("type", "button")
        .attr("class", "btn btn-primary btn-block")
        .attr("disabled", !on_click.is_ready())
        .callback("click", [&](auto& e) { perform_action(on_click); })
        .text(text);
}

void
checkbox(html::context ctx, duplex<bool> value, readable<std::string> label)
{
    bool determinate = value.has_value();
    bool checked = determinate && value.read();
    bool disabled = !value.ready_to_write();

    element(ctx, "div")
        .attr("class", "custom-control custom-checkbox")
        .children([&](auto ctx) {
            element(ctx, "input")
                .attr("type", "checkbox")
                .attr("class", "custom-control-input")
                .attr("disabled", disabled)
                .attr("id", "custom-check-1")
                .prop("indeterminate", !determinate)
                .prop("checked", checked)
                .callback("change", [&](emscripten::val e) {
                    write_signal(value, e["target"]["checked"].as<bool>());
                });
            element(ctx, "label")
                .attr("class", "custom-control-label")
                .attr("for", "custom-check-1")
                .text(label);
        });
}

element_handle<html::context>
link(html::context ctx, readable<std::string> text, action<> on_click)
{
    return element(ctx, "a")
        .attr("href", "javascript: void(0);")
        .attr("disabled", on_click.is_ready() ? "false" : "true")
        .children([&](auto ctx) { text_node(ctx, text); })
        .callback("click", [&](emscripten::val) { perform_action(on_click); });
}

} // namespace detail

} // namespace html
} // namespace alia
