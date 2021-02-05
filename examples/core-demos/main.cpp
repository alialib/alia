#include <alia/html/bootstrap.hpp>
#include <alia/html/dom.hpp>
#include <alia/html/fetch.hpp>
#include <alia/html/routing.hpp>
#include <alia/html/storage.hpp>
#include <alia/html/system.hpp>
#include <alia/html/widgets.hpp>
#include <sstream>

#include "../demo-utilities/utilities.hpp"

/// [namespace]
using namespace alia;
namespace bs = alia::html::bootstrap;
/// [namespace]

void
text_demo(demo_context ctx)
{
    section_heading(ctx, "text", "Text");
}

void
storage_demo(demo_context ctx)
{
    section_heading(ctx, "storage", "Storage");

    subsection_heading(ctx, "Local");

    p(ctx,
      "The input in this demo is connected to the HTML5 local storage API of "
      "your browser. If you edit it, close this page and reopen it, you'll "
      "see the same value.");

    p(ctx,
      "You can even open this page in a second window, edit the value there, "
      "and watch it update here.");

    div(ctx, "demo-panel", [&] {
        /// [local-storage]
        input(ctx, get_local_state(ctx, "demo-local-storage-key"));
        /// [local-storage]
    });

    code_snippet(ctx, "local-storage");

    subsection_heading(ctx, "Session");

    p(ctx,
      "The input in this demo is connected to the HTML5 session storage API "
      "of your browser. This value will survive refreshes, but it will reset "
      "if you close the page, and it's not shared across multiple instances "
      "of the page.");

    div(ctx, "demo-panel", [&] {
        /// [session-storage]
        input(ctx, get_session_state(ctx, "demo-session-storage-key"));
        /// [session-storage]
    });

    code_snippet(ctx, "session-storage");
}

void
root_ui(html::context vanilla_ctx)
{
    with_demo_context(vanilla_ctx, [&](auto ctx) {
        placeholder_root(ctx, "demos", [&] {
            h1(ctx).classes("mt-5 mb-3").text("alia/HTML Core Demos");

            p(ctx,
              "The following demonstrate some of the core capabilities of "
              "alia/HTML.");

            storage_demo(ctx);

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
