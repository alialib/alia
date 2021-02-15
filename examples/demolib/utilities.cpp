#include "utilities.hpp"

#include <alia/html/dom.hpp>
#include <alia/html/widgets.hpp>

using namespace alia;

void
section_heading(html::context ctx, char const* anchor, char const* label)
{
    h2(ctx).classes("mt-5 mb-3").children([&] {
        element(ctx, "a")
            .attr("name", anchor)
            .attr(
                "style",
                "padding-top: 112px; "
                "margin-top: -112px; "
                "display: inline-block; "
                "pointer-events: none; ")
            .text(label);
    });
}

void
subsection_heading(html::context ctx, char const* label)
{
    h4(ctx).classes("mt-4 mb-3").text(label);
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
    auto code_block
        = pre(ctx).class_("language-cpp").children([&] { code(ctx, src); });
    on_value_gain(ctx, src, callback([&] {
                      EM_ASM(
                          { Prism.highlightElement(Module.nodes[$0]); },
                          code_block.asmdom_id());
                  }));
}

void
with_demo_context(
    alia::html::context vanilla_ctx,
    alia::function_view<void(demo_context)> const& content)
{
    auto src = fetch_text(vanilla_ctx, value("main.cpp"));

    auto ctx = extend_context<src_tag>(vanilla_ctx, src);

    content(ctx);
}
