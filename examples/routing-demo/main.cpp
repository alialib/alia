#include <alia/html/bootstrap.hpp>
#include <alia/html/dom.hpp>
#include <alia/html/fetch.hpp>
#include <alia/html/routing.hpp>
#include <alia/html/system.hpp>
#include <alia/html/widgets.hpp>
#include <sstream>

#include "../demolib/utilities.hpp"

/// [namespace]
using namespace alia;
namespace bs = alia::html::bootstrap;
/// [namespace]

void
basic_routing_demo(demo_context ctx)
{
    section_heading(ctx, "basic", "Basic Routing");

    p(ctx,
      "Here's an example that does some very basic routing with static "
      "content routes.");

    code_snippet(ctx, "basic-routing");

    p(ctx, "And here it is in action:");

    div(ctx, "demo-panel", [&] {
        /// [basic-routing]
        router(ctx)
            .route("/", [&] { p(ctx, "Welcome to the routing demo!"); })
            .route("/about", [&] { p(ctx, "You've found the About page!"); })
            .route("/users", [&] {
                h4(ctx, "Users");
                p(ctx, "Calvin");
                p(ctx, "Hobbes");
            });
        /// [basic-routing]
    }).attr("style", "min-height: 120px");

    p(ctx,
      "You can control the content of that panel by typing in different "
      "hash locations in the address bar, but here are some links that will "
      "do that for you:");

    div(ctx, "demo-panel", [&] {
        /// [basic-links]
        link(ctx, "Home", "#/");
        link(ctx, "About", "#/about");
        link(ctx, "Users", "#/users");
        /// [basic-links]
    });

    p(ctx, "And the code for those:");

    code_snippet(ctx, "basic-links");
}

void
parameters_demo(demo_context ctx)
{
    section_heading(ctx, "parameters", "Route Parameters");

    p(ctx, [&] {
        text(
            ctx,
            "Routes can include parameters. "
            "These are represented by the token ");
        code(ctx, "{}");
        text(ctx, " in the pattern string.");
    });

    p(ctx,
      "Let's extend our mock 'Users' functionality to include routes "
      "for the individual users. Since users are dynamic (or at least "
      "would be in a real app), we'll need a parameter for this.");

    code_snippet(ctx, "routing-parameters");

    div(ctx, "demo-panel", [&] {
        /// [routing-parameters]
        router(ctx)
            .route(
                "/users",
                [&] {
                    h4(ctx, "Users");
                    p(ctx, "Calvin");
                    p(ctx, "Hobbes");
                })
            .route("/users/{}", [&](auto user) {
                h4(ctx, user);
                p(ctx, "Pretending to fetch info about this user...");
            });
        /// [routing-parameters]
    }).attr("style", "min-height: 120px");

    p(ctx, "Now our demo works with routes like this:");

    div(ctx, "demo-panel", [&] {
        link(ctx, "/users", "#/users");
        link(ctx, "/users/calvin", "#/users/calvin");
        link(ctx, "/users/hobbes", "#/users/hobbes");
    });
}

void
subpaths_demo(demo_context ctx)
{
    section_heading(ctx, "subpaths", "Subpaths");

    p(ctx, [&] {
        text(ctx, "While ");
        code(ctx, "{}");
        text(ctx, " will only match one level in the route path, the token ");
        code(ctx, "{:/}");
        text(
            ctx,
            " can match multiple levels (i.e., the string that it matches can "
            "include the ");
        code(ctx, "/");
        text(ctx, " character.");
    });

    p(ctx,
      "This allows you to break down complex route matching code into more "
      "manageable pieces:");

    code_snippet(ctx, "routing-subpaths");

    div(ctx, "demo-panel", [&] {
        /// [routing-subpaths]
        router(ctx).route("/accounts/{}{:/}", [&](auto account, auto subpath) {
            h4(ctx, account);
            // Set up another router to match against the subpath.
            // (In real code, we might split this out into its own function.)
            router(ctx, subpath)
                .route(
                    "/",
                    [&] {
                        p(ctx,
                          "Pretending to fetch info on " + account + "...");
                    })
                .route("/users", [&] {
                    p(ctx, "Pretending to fetch users in " + account + "...");
                });
        });
        /// [routing-subpaths]
    }).attr("style", "min-height: 120px");

    p(ctx, "Now our demo works with routes like this:");

    div(ctx, "demo-panel", [&] {
        link(ctx, "/accounts/acme/", "#/accounts/acme/");
        link(ctx, "/accounts/alia/users", "#/accounts/alia/users");
    });
}

void
default_match_demo(demo_context ctx)
{
    section_heading(ctx, "default", "The Default Case");

    p(ctx, [&] {
        code(ctx, "\"{:/}\"");
        text(
            ctx,
            " as a pattern string by itself will match anything, so you can "
            "use it at the end of your router to catch anything that wasn't "
            "matched by an earlier pattern.");
    });

    code_snippet(ctx, "default-case");

    div(ctx, "demo-panel", [&] {
        /// [default-case]
        router(ctx)
            .route("/about", [&] { p(ctx, "You've found the About page!"); })
            .route("{:/}", [&](auto path) {
                h4(ctx, "404");
                p(ctx, path + " not found");
            });
        /// [default-case]
    }).attr("style", "min-height: 120px");

    p(ctx, "Try it:");

    div(ctx, "demo-panel", [&] {
        link(ctx, "/about", "#/about");
        link(ctx, "/unsupported-path", "#/unsupported-path");
    });
}

void
root_ui(html::context vanilla_ctx)
{
    with_demo_context(vanilla_ctx, [&](auto ctx) {
        placeholder_root(ctx, "demos", [&] {
            h1(ctx).classes("mt-5 mb-3").text("alia/HTML Routing");

            p(ctx,
              "The following is a demonstration of alia/HTML's hash-based "
              "routing capabilities. For demonstration purposes, we're "
              "using the hash location to control just the content inside a "
              "few small panels. However, you can use the exact same "
              "technique to control any subset of your app's content, even "
              "the entire page.");

            basic_routing_demo(ctx);
            parameters_demo(ctx);
            subpaths_demo(ctx);
            default_match_demo(ctx);

            div(ctx, "my-5");
        });
    });
}

int
main()
{
    static html::system sys;
    initialize(sys, root_ui);
    enable_hash_monitoring(sys);
};
