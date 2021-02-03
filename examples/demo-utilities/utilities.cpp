#include "utilities.hpp"

#include <alia/html/dom.hpp>

using namespace alia;

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
with_demo_context(
    alia::html::context vanilla_ctx,
    alia::function_view<void(demo_context)> const& content)
{
    auto src = fetch_text(vanilla_ctx, value("main.cpp"));

    auto ctx = extend_context<src_tag>(vanilla_ctx, src);

    content(ctx);
}
