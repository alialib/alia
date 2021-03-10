#include <alia/html/bootstrap.hpp>
#include <alia/html/document.hpp>
#include <alia/html/dom.hpp>
#include <alia/html/fetch.hpp>
#include <alia/html/routing.hpp>
#include <alia/html/storage.hpp>
#include <alia/html/system.hpp>
#include <alia/html/widgets.hpp>
#include <sstream>

#include "../demolib/utilities.hpp"

using namespace alia;
namespace bs = alia::html::bootstrap;

void
text_demo(demo_context ctx)
{
    section_heading(ctx, "text", "Text");

    subsection_heading(ctx, "Headings");
    div(ctx, "demo-panel", [&] {
        /// [headings]
        h1(ctx, "Heading 1");
        h2(ctx, "Heading 2");
        h3(ctx, "Heading 3");
        h4(ctx, "Heading 4");
        h5(ctx, "Heading 5");
        h6(ctx, "Heading 6");
        /// [headings]
    });
    code_snippet(ctx, "headings");

    subsection_heading(ctx, "Paragraphs");
    div(ctx, "demo-panel", [&] {
        /// [paragraphs]
        p(ctx,
          "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
          "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
          "enim ad minim veniam, quis nostrud exercitation ullamco laboris "
          "nisi ut aliquip ex ea commodo consequat. ");
        /// [paragraphs]
    });
    code_snippet(ctx, "paragraphs");

    subsection_heading(ctx, "Formatted Text");
    p(ctx, "Directly as HTML...");
    div(ctx, "demo-panel", [&] {
        // clang-format off
        /// [formatted-text-html]
        html_fragment(ctx,
            R"(
            <p>
                <code>&lt;p&gt;</code> elements can also serve as
                <i>containers,</i> and you can <b>mix in</b> other style of
                text.
            </p>
            )");
        /// [formatted-text-html]
        // clang-format on
    });
    code_snippet(ctx, "formatted-text-html");

    p(ctx, "In C++...");
    div(ctx, "demo-panel", [&] {
        /// [formatted-text-cpp]
        p(ctx, [&] {
            code(ctx, "<p>");
            text(ctx, " elements can also serve as ");
            i(ctx, "containers,");
            text(ctx, " and you can ");
            b(ctx, "mix in");
            text(ctx, " other style of text.");
        });
        /// [formatted-text-cpp]
    });
    code_snippet(ctx, "formatted-text-cpp");
}

void
checkboxes_demo(demo_context ctx)
{
    section_heading(ctx, "checkboxes", "Checkboxes");

    p(ctx,
      "alia/HTML provides more natural interfaces to elements like "
      "checkboxes.");

    subsection_heading(ctx, "Normal");
    p(ctx,
      "Checkboxes can connect directly to signals to display and manipulate "
      "their values.");
    div(ctx, "demo-panel", [&] {
        /// [checkbox]
        auto my_state = get_state(ctx, false);
        checkbox(ctx, my_state);
        alia_if(my_state)
        {
            p(ctx, "Box is checked!");
        }
        alia_end
        /// [checkbox]
    });
    code_snippet(ctx, "checkbox");

    subsection_heading(ctx, "Disabled");
    p(ctx,
      "Checkboxes automatically disable themselves when the associated signal "
      "isn't ready to write.");
    div(ctx, "demo-panel", [&] {
        /// [disabled-checkbox]
        auto my_state = get_state(ctx, false);
        checkbox(ctx, disable_writes(my_state));
        /// [disabled-checkbox]
    });
    code_snippet(ctx, "disabled-checkbox");

    subsection_heading(ctx, "Indeterminate");
    p(ctx,
      "Checkboxes go into the indeterminate state when the associated signal "
      "doesn't carry a value.");
    div(ctx, "demo-panel", [&] {
        /// [indeterminate-checkbox]
        auto my_state = get_state(ctx, empty<bool>());
        checkbox(ctx, my_state);
        /// [indeterminate-checkbox]
    });
    code_snippet(ctx, "indeterminate-checkbox");
}

void
root_ui(html::context vanilla_ctx)
{
    with_demo_context(vanilla_ctx, [&](auto ctx) {
        document_title(ctx, "alia/HTML Element Demos");

        placeholder_root(ctx, "demos", [&] {
            h1(ctx).classes("mt-5 mb-3").text("alia/HTML Element Demos");

            p(ctx,
              "The following demonstrate alia/HTML's interface to some of the "
              "core HTML elements.");

            text_demo(ctx);
            checkboxes_demo(ctx);

            div(ctx, "my-5");
        });
    });
}

int
main()
{
    static html::system sys;
    initialize(sys, root_ui);
};
