#include <alia/html/bootstrap.hpp>
#include <alia/html/dom.hpp>
#include <alia/html/fetch.hpp>
#include <alia/html/routing.hpp>
#include <alia/html/system.hpp>
#include <alia/html/widgets.hpp>
#include <sstream>

#include "../demo-utilities/utilities.hpp"

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

    div(ctx, "demo-panel routed-content", [&] {
        /// [basic-routing]
        router(ctx)
            .route("/", [&] { p(ctx, "Welcome to the routing demo!"); })
            .route("/about", [&] { p(ctx, "You've found the About page!"); })
            .route("/users", [&] {
                element(ctx, "h4").text("Users");
                p(ctx, "Calvin");
                p(ctx, "Hobbes");
            });
        /// [basic-routing]
    });

    p(ctx,
      "You can control the content of that panel by typing in different "
      "hash locations in the address bar, but here are some links that will "
      "do that for you:");

    div(ctx, "demo-panel", [&] {
        /// [basic-links]
        internal_link(ctx, "Home", "/");
        internal_link(ctx, "About", "/about");
        internal_link(ctx, "Users", "/users");
        /// [basic-links]
    });

    p(ctx, "And the code for those:");

    code_snippet(ctx, "basic-links");
}

void
parameters_demo(demo_context ctx)
{
    section_heading(ctx, "parameters", "Route Parameters");

    element(ctx, "p").children([&] {
        text_node(
            ctx,
            "Routes can include parameters. "
            "These are represented by the token ");
        element(ctx, "code").text("{}");
        text_node(ctx, " in the pattern string.");
    });

    p(ctx,
      "Let's extend our mock 'Users' functionality to include routes "
      "for the individual users. Since users are dynamic (or at least "
      "would be in a real app), we'll need a parameter for this.");

    code_snippet(ctx, "routing-parameters");

    div(ctx, "demo-panel routed-content", [&] {
        /// [routing-parameters]
        router(ctx)
            .route(
                "/users",
                [&] {
                    element(ctx, "h4").text("Users");
                    p(ctx, "Calvin");
                    p(ctx, "Hobbes");
                })
            .route("/users/{}", [&](auto user) {
                element(ctx, "h4").text(user);
                p(ctx, "Pretending to fetch info about this user...");
            });
        /// [routing-parameters]
    });

    p(ctx, "Now our demo works with routes like this:");

    div(ctx, "demo-panel", [&] {
        internal_link(ctx, "/users", "/users");
        internal_link(ctx, "/users/calvin", "/users/calvin");
        internal_link(ctx, "/users/hobbes", "/users/hobbes");
    });
}

void
subpaths_demo(demo_context ctx)
{
    section_heading(ctx, "subpaths", "Subpaths");

    element(ctx, "p").children([&] {
        text_node(ctx, "While ");
        element(ctx, "code").text("{}");
        text_node(
            ctx, " will only match one level in the route path, the token ");
        element(ctx, "code").text("{:/}");
        text_node(
            ctx,
            " can match multiple levels (i.e., the string that it matches can "
            "include the ");
        element(ctx, "code").text("/");
        text_node(ctx, " character.");
    });

    p(ctx,
      "This allows you to break down complex route matching code into more "
      "manageable pieces:");

    code_snippet(ctx, "routing-subpaths");

    div(ctx, "demo-panel routed-content", [&] {
        /// [routing-subpaths]
        router(ctx).route("/accounts/{}{:/}", [&](auto account, auto subpath) {
            element(ctx, "h4").text(account);
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
    });

    p(ctx, "Now our demo works with routes like this:");

    div(ctx, "demo-panel", [&] {
        internal_link(ctx, "/accounts/acme/", "/accounts/acme/");
        internal_link(ctx, "/accounts/alia/users", "/accounts/alia/users");
    });
}

void
default_match_demo(demo_context ctx)
{
    section_heading(ctx, "default", "The Default Case");

    element(ctx, "p").children([&] {
        element(ctx, "code").text("\"{:/}\"");
        text_node(
            ctx,
            " as a pattern string by itself will match anything, so you can "
            "use it at the end of your router to catch anything that wasn't "
            "matched by an earlier pattern.");
    });

    code_snippet(ctx, "default-case");

    div(ctx, "demo-panel routed-content", [&] {
        /// [default-case]
        router(ctx)
            .route("/about", [&] { p(ctx, "You've found the About page!"); })
            .route("{:/}", [&](auto path) {
                element(ctx, "h4").text("404");
                p(ctx, path + " not found");
            });
        /// [default-case]
    });

    p(ctx, "Try it:");

    div(ctx, "demo-panel", [&] {
        internal_link(ctx, "/about", "/about");
        internal_link(ctx, "/unsupported-path", "/unsupported-path");
    });
}

void
root_ui(html::context vanilla_ctx)
{
    with_demo_context(vanilla_ctx, [&](auto ctx) {
        placeholder_root(ctx, "demos", [&] {
            element(ctx, "h1").classes("mt-5 mb-3").text("alia/HTML Routing");

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
