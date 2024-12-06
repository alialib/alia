#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/spacer.hpp>
#include <alia/ui/library/collapsible.hpp>
#include <alia/ui/library/node_expander.hpp>
#include <alia/ui/scrolling.hpp>
#include <alia/ui/text/components.hpp>

#include <stacktrace>

#include <cpp-tree-sitter.h>

extern "C" {
const TSLanguage*
tree_sitter_cpp(void);
}

using namespace alia;

auto my_style = text_style{"Roboto/Roboto-Regular", 22.f, rgb8(173, 181, 189)};

int
factor(int n)
{
    int i;
    for (i = int(std::sqrt(n) + 0.5); i > 1 && n % i != 0; --i)
        ;
    return i;
}

void
factor_tree(ui_context ctx, readable<int> n)
{
    // Get the 'best' factor that n has. (The one closest to sqrt(n).)
    auto f = alia::apply(ctx, factor, n);

    // If that factor isn't 1, n is composite.
    if_(ctx, f != 1, [&] {
        auto show_factors = get_state(ctx, false);

        {
            row_layout row(ctx);
            do_node_expander(ctx, show_factors);
            do_text(
                ctx, direct(my_style), alia::printf(ctx, "%i: composite", n));
        }

        collapsible_content(ctx, show_factors, GROW, [&] {
            row_layout row(ctx);
            do_spacer(ctx, width(40, PIXELS));
            column_layout column(ctx, GROW);
            factor_tree(ctx, f);
            factor_tree(ctx, n / f);
        });
    }).else_([&] {
        row_layout row(ctx);
        do_spacer(ctx, width(40, PIXELS));
        do_text(ctx, direct(my_style), alia::printf(ctx, "%i: prime", n));
    });
}

void
my_ui(ui_context ctx)
{
    scoped_scrollable_view scrollable(ctx, GROW);

    column_layout column(ctx, GROW | PADDED);

    factor_tree(ctx, value(768'000'000));

    do_spacer(ctx, height(20, PIXELS));

    // Create a tree-sitter parser for C++
    auto parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_cpp());

    // Parse the source code
    const char* source = R"(
void
factor_tree(ui_context ctx, readable<int> n)
{
    // Get the 'best' factor that n has. (The one closest to sqrt(n).)
    auto f = alia::apply(ctx, factor, n);

    // If that factor isn't 1, n is composite.
    if_(ctx, f != 1, [&] {
        auto show_factors = get_state(ctx, false);

        {
            row_layout row(ctx);
            do_node_expander(ctx, show_factors);
            do_text(
                ctx, direct(my_style), alia::printf(ctx, "%i: composite", n));
        }

        collapsible_content(ctx, show_factors, GROW, [&] {
            row_layout row(ctx);
            do_spacer(ctx, width(40, PIXELS));
            column_layout column(ctx, GROW);
            factor_tree(ctx, f);
            factor_tree(ctx, n / f);
        });
    }).else_([&] {
        row_layout row(ctx);
        do_spacer(ctx, width(40, PIXELS));
        do_text(ctx, direct(my_style), alia::printf(ctx, "%i: prime", n));
    });
}
    )";

    TSTree* tree = ts_parser_parse_string(
        parser, nullptr, source, static_cast<uint32_t>(strlen(source)));

    // Get the root node
    TSNode root = ts_tree_root_node(tree);

    // Function to recursively traverse and display the AST
    std::function<void(TSNode, int)> display_node = [&](TSNode node,
                                                        int depth) {
        // Create indentation
        std::string indent(depth * 2, ' ');

        // Get node text content
        uint32_t start_byte = ts_node_start_byte(node);
        uint32_t end_byte = ts_node_end_byte(node);
        std::string node_text(source + start_byte, end_byte - start_byte);

        // Display node with syntax highlighting
        alia_if (ts_node_is_named(node))
        {
            do_text(
                ctx,
                direct(my_style),
                alia::printf(
                    ctx, "%s%s: %s", indent, ts_node_type(node), node_text));
        }
        alia_else_if (!node_text.empty() && node_text[0] != '\n')
        {
            do_text(
                ctx,
                direct(my_style),
                alia::printf(ctx, "%s'%s'", indent, node_text));
        }
        alia_end

        // Recursively display children
        uint32_t child_count = ts_node_child_count(node);
        alia_for(uint32_t i = 0; i < child_count; i++)
        {
            TSNode child = ts_node_child(node, i);
            display_node(child, depth + 1);
        }
        alia_end
    };

    // Start displaying from root
    display_node(root, 0);

    // Cleanup
    ts_tree_delete(tree);
    ts_parser_delete(parser);
}
