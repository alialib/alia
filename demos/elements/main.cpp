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
    div(ctx, "demo-panel", [&] {
        /// [formatted-text]
        p(ctx, [&] {
            text(ctx, "Paragraphs can also serve as ");
            i(ctx, "containers,");
            text(ctx, " and you can ");
            b(ctx, "mix in");
            text(ctx, " other ");
            code(ctx, "styles");
            text(ctx, " of ");
            ins(ctx, "text.");
        });
        /// [formatted-text]
    });
    code_snippet(ctx, "formatted-text");
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
