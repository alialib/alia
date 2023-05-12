#include "demo.hpp"

#include <alia/html/fetch.hpp>

namespace basic_element {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [basic-element]
element(ctx, "hr");
/// [basic-element]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("basic-element", init_demo);

} // namespace basic_element

namespace shorter_basic_element {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [shorter-basic-element]
hr(ctx);
/// [shorter-basic-element]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("shorter-basic-element", init_demo);

} // namespace shorter_basic_element

namespace element_attribute {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [element-attribute]
hr(ctx).attr("width", "50%");
/// [element-attribute]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("element-attribute", init_demo);

} // namespace element_attribute

namespace valueless_element_attribute {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [valueless-element-attribute]
input(ctx).attr("type", "checkbox").attr("checked").attr("disabled");
/// [valueless-element-attribute]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("valueless-element-attribute", init_demo);

} // namespace valueless_element_attribute

namespace boolean_element_attribute {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [boolean-element-attribute]
auto disabled = get_state(ctx, false);
input(ctx).attr("type", "checkbox").attr("checked").attr("disabled", disabled);
button(ctx, "Toggle", actions::toggle(disabled));
/// [boolean-element-attribute]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("boolean-element-attribute", init_demo);

} // namespace boolean_element_attribute

namespace simple_element_property {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [simple-element-property]
input(ctx).attr("type", "checkbox").prop("checked", true);
/// [simple-element-property]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("simple-element-property", init_demo);

} // namespace simple_element_property

namespace dynamic_element_property {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [dynamic-element-property]
auto checked = get_state(ctx, false);
input(ctx).attr("type", "checkbox").prop("checked", checked);
button(ctx, "Toggle", actions::toggle(checked));
/// [dynamic-element-property]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("dynamic-element-property", init_demo);

} // namespace dynamic_element_property

namespace element_handler {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [element-handler]
auto checked = get_state(ctx, false);
input(ctx)
    .attr("type", "checkbox")
    .prop("checked", checked)
    .handler("change", [&](emscripten::val e) {
        write_signal(checked, e["target"]["checked"].as<bool>());
    });
button(ctx, "Toggle", actions::toggle(checked));
/// [element-handler]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("element-handler", init_demo);

} // namespace element_handler

namespace element_action {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [element-action]
auto checked = get_state(ctx, false);
input(ctx)
    .attr("type", "checkbox")
    .prop("checked", checked)
    .on("change", actions::toggle(checked));
button(ctx, "Toggle", actions::toggle(checked));
/// [element-action]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("element-action", init_demo);

} // namespace element_action

namespace element_text {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [element-text]
p(ctx).text("This is a paragraph of text.");
/// [element-text]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("element-text", init_demo);

} // namespace element_text

namespace element_text_node {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [element-text-node]
text(ctx, "This is a freestanding HTML text node.");
/// [element-text-node]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("element-text-node", init_demo);

} // namespace element_text_node

namespace element_content {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [element-content]
span(ctx).content([&] {
    text(ctx, "This is some normal text. ");
    b(ctx, "This is some bold text.");
});
/// [element-content]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("element-content", init_demo);

} // namespace element_content

namespace simple_html_fragment {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [simple-html-fragment]
html_fragment(ctx, "<p>Hello, <em>World!</em></p>");
/// [simple-html-fragment]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("simple-html-fragment", init_demo);

} // namespace simple_html_fragment

namespace dynamic_html_fragment {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [dynamic-html-fragment]
auto name = get_state(ctx, "World");
input(ctx, name);
html_fragment(ctx, printf(ctx, "<p>Hello, <em>%s!</em></p>", name));
/// [dynamic-html-fragment]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("dynamic-html-fragment", init_demo);

} // namespace dynamic_html_fragment

namespace fetched_html_fragment {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [fetched-html-fragment]
html_fragment(ctx, fetch_text(ctx, "fragment.html"));
/// [fetched-html-fragment]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("fetched-html-fragment", init_demo);

} // namespace fetched_html_fragment

namespace overridden_html_fragment {

void
demo_ui(html::context ctx)
{
    // clang-format off
/// [overridden-html-fragment]
auto table = html_fragment(ctx, R"HTML(
    <table style="width:100%">
        <tr>
            <th>Name</th>
            <th>Age</th>
        </tr>
        <tr id="content">
    </table>
)HTML");
table.override("content", [&] {
    tr(ctx, [&] {
        td(ctx, "John");
        td(ctx, "21");
    });
    tr(ctx, [&] {
        td(ctx, "Mary");
        td(ctx, "27");
    });
});
/// [overridden-html-fragment]
    // clang-format on
}

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(the_system, dom_id, [](html::context ctx) { demo_ui(ctx); });
}

static demo the_demo("overridden-html-fragment", init_demo);

} // namespace overridden_html_fragment
