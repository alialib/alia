#include <alia/html/bootstrap.hpp>
#include <alia/html/dom.hpp>
#include <alia/html/fetch.hpp>
#include <alia/html/system.hpp>
#include <alia/html/widgets.hpp>
#include <sstream>

#include "../demo-utilities/utilities.hpp"

/// [namespace]
using namespace alia;
namespace bs = alia::html::bootstrap;
/// [namespace]

void
bootstrap_docs_link(html::context ctx, char const* path)
{
    link(
        ctx,
        "Bootstrap docs",
        "https://getbootstrap.com/docs/4.5/" + value(path))
        .classes("bs-docs-link d-inline-block mb-3")
        .attr("target", "_blank")
        .attr("rel", "noopener noreferrer");
}

void
section_heading(
    html::context ctx,
    char const* anchor,
    char const* label,
    char const* bs_docs_path)
{
    section_heading(ctx, anchor, label);
    bootstrap_docs_link(ctx, bs_docs_path);
}

void
breadcrumb_demo(demo_context ctx)
{
    section_heading(
        ctx, "breadcrumbs", "Breadcrumbs", "components/breadcrumb/");
    /// [breadcrumb]
    {
        bs::breadcrumb bc(ctx);
        bc.item([&] { link(ctx, "Home", actions::noop()); });
        bc.item([&] { link(ctx, "Library", actions::noop()); });
        bc.active_item("Data");
    }
    /// [breadcrumb]
    code_snippet(ctx, "breadcrumb");
}

void
buttons_demo(demo_context ctx)
{
    section_heading(ctx, "buttons", "Buttons", "components/buttons/");

    subsection_heading(ctx, "Normal");
    div(ctx, "demo-panel", [&] {
        /// [normal-buttons]
        bs::primary_button(ctx, "Primary", actions::noop());
        bs::secondary_button(ctx, "Secondary", actions::noop());
        bs::success_button(ctx, "Success", actions::noop());
        bs::danger_button(ctx, "Danger", actions::noop());
        bs::warning_button(ctx, "Warning", actions::noop());
        bs::info_button(ctx, "Info", actions::noop());
        bs::light_button(ctx, "Light", actions::noop());
        bs::dark_button(ctx, "Dark", actions::noop());
        bs::link_button(ctx, "Link", actions::noop());
        /// [normal-buttons]
    });
    code_snippet(ctx, "normal-buttons");

    subsection_heading(ctx, "Outline");
    div(ctx, "demo-panel", [&] {
        /// [outline-buttons]
        bs::outline_primary_button(ctx, "Primary", actions::noop());
        bs::outline_secondary_button(ctx, "Secondary", actions::noop());
        bs::outline_success_button(ctx, "Success", actions::noop());
        bs::outline_danger_button(ctx, "Danger", actions::noop());
        bs::outline_warning_button(ctx, "Warning", actions::noop());
        bs::outline_info_button(ctx, "Info", actions::noop());
        bs::outline_light_button(ctx, "Light", actions::noop());
        bs::outline_dark_button(ctx, "Dark", actions::noop());
        /// [outline-buttons]
    });
    code_snippet(ctx, "outline-buttons");
}

void
modals_demo(demo_context ctx)
{
    section_heading(ctx, "modals", "Modals", "components/modal/");

    subsection_heading(ctx, "Simple");
    div(ctx, "demo-panel", [&] {
        /// [simple-modal]
        auto modal = bs::modal(ctx, [&](auto modal) {
            modal.title("Simple Modal");
            modal.body([&] { p(ctx, "There's not much to see here."); });
            modal.footer([&] {
                bs::primary_button(ctx, "Close", modal.close_action());
            });
        });

        bs::primary_button(ctx, "Activate", modal.activate_action());
        /// [simple-modal]
    });
    code_snippet(ctx, "simple-modal");

    subsection_heading(ctx, "w/Fade Effect");
    p(ctx,
      "The handle that you get for the modal is also a normal element handle "
      "for the top-level modal element, so you can add in Bootstrap classes "
      "to enable effects like fading.");
    div(ctx, "demo-panel", [&] {
        /// [fading-modal]
        auto modal = bs::modal(ctx, [&](auto modal) {
            modal.title("Fading Modal");
            modal.body([&] { p(ctx, "There's still not much to see here."); });
            modal.footer([&] {
                bs::primary_button(ctx, "Close", modal.close_action());
            });
        });

        // Add the 'fade' class.
        // Note that this needs to be done outside the modal itself so that
        // Bootstrap actually sees the class before the modal is opened.
        modal.class_("fade");

        bs::primary_button(ctx, "Activate", modal.activate_action());
        /// [fading-modal]
    });
    code_snippet(ctx, "fading-modal");

    subsection_heading(ctx, "w/Shared State");
    p(ctx,
      "Since modals are defined within the components that invoke them, they "
      "can naturally access the state from those components. Here's an "
      "example of using a modal to do a cancelable edit of state from the "
      "parent component.");
    // clang-format off
    div(ctx, "demo-panel", [&] {
        /// [shared-state-modal]
        p(ctx, "Here's some state that we'll edit inside the modal. "
            "Feel free to edit it out here too:");
        auto my_state = get_state(ctx, "Edit me!");
        input(ctx, my_state);

        auto modal = bs::modal(ctx, [&](auto modal) {
            modal.title("State Sharing Modal");

            // Create a local copy of the state that we can edit (and
            // potentially discard) within the modal.
            // We use transient state here so that it's reset whenever the
            // modal reopens.
            auto local_copy = get_transient_state(ctx, my_state);

            modal.body([&] {
                p(ctx, "Here's a local copy of the state to edit.");
                p(ctx, "OK this dialog to save the changes.");
                input(ctx, local_copy);
            });

            modal.footer([&] {
                bs::secondary_button(ctx, "Cancel", modal.close_action());
                bs::primary_button(ctx, "OK",
                    (my_state <<= local_copy, modal.close_action()));
            });
        });

        bs::primary_button(ctx, "Activate", modal.activate_action());
        /// [shared-state-modal]
    });
    // clang-format on
    code_snippet(ctx, "shared-state-modal");
}

void
tooltips_demo(demo_context ctx)
{
    section_heading(ctx, "tooltips", "Tooltips", "components/tooltips/");

    subsection_heading(ctx, "On Buttons");
    element(ctx, "p").children([&] {
        text_node(ctx, "Buttons directly provide a ");
        element(ctx, "code").text(".tooltip()");
        text_node(ctx, " member.");
    });
    div(ctx, "demo-panel", [&] {
        /// [button-tooltips]
        bs::dark_button(ctx, "Button w/tooltip", actions::noop())
            .tooltip("Automatically positioned tooltip");
        bs::dark_button(ctx, "w/tooltip on top", actions::noop())
            .tooltip("Tooltip on top", "top");
        bs::dark_button(ctx, "w/tooltip on bottom", actions::noop())
            .tooltip("Tooltip on bottom", "bottom");
        bs::dark_button(ctx, "w/tooltip on left", actions::noop())
            .tooltip("Tooltip on left", "left");
        bs::dark_button(ctx, "w/tooltip on right", actions::noop())
            .tooltip("Tooltip on right", "right");
        /// [button-tooltips]
    });
    code_snippet(ctx, "button-tooltips");

    subsection_heading(ctx, "In General");
    element(ctx, "p").children([&] {
        text_node(ctx, "You can also use the free function ");
        element(ctx, "code").text("bootstrap::tooltip()");
        text_node(ctx, " to add tooltips to other elements.");
    });
    div(ctx, "demo-panel", [&] {
        /// [general-tooltips]
        auto my_link = link(ctx, "link w/tooltip", actions::noop());
        bs::tooltip(my_link, "Tooltip on link");
        /// [general-tooltips]
    });
    code_snippet(ctx, "general-tooltips");
}

void
root_ui(html::context vanilla_ctx)
{
    with_demo_context(vanilla_ctx, [&](auto ctx) {
        placeholder_root(ctx, "demos", [&] {
            p(ctx,
              "All code snippets assume the following namespace "
              "declarations are in effect:");
            code_snippet(ctx, "namespace");

            breadcrumb_demo(ctx);
            buttons_demo(ctx);
            modals_demo(ctx);
            tooltips_demo(ctx);
        });
    });
}

int
main()
{
    static html::system the_sys;
    initialize(the_sys, root_ui);
};
