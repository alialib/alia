#include <alia/html/bootstrap.hpp>
#include <alia/html/dom.hpp>
#include <alia/html/fetch.hpp>
#include <alia/html/system.hpp>
#include <alia/html/widgets.hpp>
#include <sstream>

/// [namespace]
using namespace alia;
namespace bs = alia::html::bootstrap;
/// [namespace]

ALIA_DEFINE_TAGGED_TYPE(src_tag, apply_signal<std::string>&)

typedef extend_context_type_t<html::context, src_tag> demo_context;

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
    element(ctx, "h2").classes("mt-5 mb-3").children([&] {
        element(ctx, "a")
            .attr("name", anchor)
            .attr(
                "style",
                "padding-top: 112px; margin-top: -112px; "
                "display: inline-block;")
            .text(label);
    });
    bootstrap_docs_link(ctx, bs_docs_path);
}

void
subsection_heading(html::context ctx, char const* label)
{
    element(ctx, "h4").classes("mt-4 mb-3").text(label);
}

std::string
extract_code_snippet(std::string const& code, std::string const& tag)
{
    std::string marker = "/// [" + tag + "]";
    auto start = code.find(marker);
    if (start == std::string::npos)
        return "Whoops! The code snippet [" + tag + "] is missing!";

    start += marker.length() + 1;
    auto end = code.find(marker, start);
    if (end == std::string::npos)
        return "Whoops! The code snippet [" + tag + "] is missing!";

    // Determine the indentation. We assume the first line of the code snippet
    // is the least indented (or at least tied for that distinction).
    int indentation = 0;
    while (std::isspace(code[start + indentation]))
        ++indentation;

    std::ostringstream snippet;
    auto i = start;
    while (true)
    {
        // Skip the indentation.
        int x = 0;
        while (x < indentation && code[i + x] == ' ')
            ++x;
        i += x;

        if (i >= end)
            break;

        // Copy the actual code line.
        while (code[i] != '\n')
        {
            snippet << code[i];
            ++i;
        }
        snippet << '\n';
        ++i;
    }

    return snippet.str();
}

void
code_snippet(demo_context ctx, char const* tag)
{
    auto src = apply(ctx, extract_code_snippet, get<src_tag>(ctx), value(tag));
    auto code = element(ctx, "pre").class_("language-cpp").children([&] {
        element(ctx, "code").text(src);
    });
    on_value_gain(ctx, src, callback([&] {
                      EM_ASM(
                          { Prism.highlightElement(Module.nodes[$0]); },
                          code.asmdom_id());
                  }));
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
        auto my_modal = bs::modal(ctx, [&](auto modal) {
            modal.title("Simple Modal");
            modal.body([&] { p(ctx, "There's not much to see here."); });
            modal.footer([&] {
                bs::primary_button(ctx, "Close", modal.close_action());
            });
        });
        bs::primary_button(ctx, "Activate", my_modal.activate_action());
        /// [simple-modal]
    });
    code_snippet(ctx, "simple-modal");

    subsection_heading(ctx, "w/Shared State");
    // clang-format off
    div(ctx, "demo-panel", [&] {
        /// [shared-state-modal]
        auto my_state = get_state(ctx, "Edit me!");
        p(ctx, "Here's some state that we'll pass to the modal for editing:");
        input(ctx, my_state);
        auto my_modal = bs::modal(ctx, [&](auto modal) {
            modal.title("State Sharing Modal");
            // The state is naturally visible in here, so just make a local
            // copy of it.
            auto local_copy = get_transient_state(ctx, my_state);
            modal.body([&] {
                p(ctx, "Here's a local copy of the state to edit.");
                p(ctx, "OK this dialog to save the changes.");
                input(ctx, local_copy);
            });
            modal.footer([&] {
                bs::link_button(ctx, "Cancel", modal.close_action());
                bs::primary_button(ctx, "OK",
                    (my_state <<= local_copy, modal.close_action()));
            });
        });
        // Add a fade effect.
        my_modal.class_("fade");
        bs::primary_button(ctx, "Activate", my_modal.activate_action());
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
        bs::primary_button(ctx, "Button w/tooltip", actions::noop())
            .tooltip("Automatically positioned tooltip");
        bs::primary_button(ctx, "w/tooltip on top", actions::noop())
            .tooltip("Tooltip on top", "top");
        bs::primary_button(ctx, "w/tooltip on bottom", actions::noop())
            .tooltip("Tooltip on bottom", "bottom");
        bs::primary_button(ctx, "w/tooltip on left", actions::noop())
            .tooltip("Tooltip on left", "left");
        bs::primary_button(ctx, "w/tooltip on right", actions::noop())
            .tooltip("Tooltip on right", "right");
        /// [button-tooltips]
    });
    code_snippet(ctx, "button-tooltips");

    subsection_heading(ctx, "In General");
    element(ctx, "p").children([&] {
        text_node(ctx, "You can also use the free function ");
        element(ctx, "code").text("tooltip()");
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
    auto src = fetch_text(vanilla_ctx, value("main.cpp"));

    auto ctx = extend_context<src_tag>(vanilla_ctx, src);

    placeholder_root(ctx, "demos", [&] {
        div(ctx, "container", [&] {
            div(ctx, "row", [&] {
                div(ctx, "col-12", [&] {
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
        });
    });
}

int
main()
{
    static html::system the_sys;
    initialize(the_sys, root_ui);
};
