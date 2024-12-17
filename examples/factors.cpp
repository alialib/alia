#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/spacer.hpp>
#include <alia/ui/library/collapsible.hpp>
#include <alia/ui/library/node_expander.hpp>
#include <alia/ui/scrolling.hpp>
#include <alia/ui/text/components.hpp>

#include <stacktrace>

#include <alia/core/system/introspection.hpp>

#include <cpp-tree-sitter.h>

extern "C" {
const TSLanguage*
tree_sitter_cpp(void);
}

using namespace alia;

// auto my_style = text_style{"Roboto/Roboto-Regular", 22.f, rgb8(173, 181,
// 189)};

introspection_traversal* the_traversal;
introspection_node root_node;

int
factor(int n)
{
    int i;
    for (i = int(std::sqrt(n) + 0.5); i > 1 && n % i != 0; --i)
        ;
    return i;
}

auto
introspective_apply(ui_context ctx, auto&& f, auto&&... args)
{
    introspection_node* node;
    if (get_data(ctx, &node))
    {
        node->name = "apply";
        node->trace = std::stacktrace::current();
    }
    if (is_refresh_event(ctx))
        add_introspection_node(*the_traversal, node);
    return alia::apply(
        ctx,
        std::forward<decltype(f)>(f),
        std::forward<decltype(args)>(args)...);
}

void
factor_tree(ui_context ctx, readable<int> n)
{
    // Get the 'best' factor that n has. (The one closest to sqrt(n).)
    auto f = introspective_apply(ctx, factor, n);

    // If that factor isn't 1, n is composite.
    alia_if (f != 1)
    {
        auto show_factors = get_state(ctx, false);

        {
            row_layout row(ctx);
            do_node_expander(ctx, show_factors);
            do_text(ctx, alia::printf(ctx, "%i: composite", n));
        }

        collapsible_content(ctx, show_factors, GROW, [&] {
            row_layout row(ctx);
            do_spacer(ctx, width(40, PIXELS));
            column_layout column(ctx, GROW);
            factor_tree(ctx, f);
            factor_tree(ctx, n / f);
        });
    }
    alia_else // n is prime.
    {
        row_layout row(ctx);
        do_spacer(ctx, width(40, PIXELS));
        do_text(ctx, direct(my_style), alia::printf(ctx, "%i: prime", n));
    }
    alia_end
}

void
app_ui(ui_context ctx)
{
    scoped_scrollable_view scrollable(ctx, GROW);

    column_layout column(ctx, GROW | PADDED);

    factor_tree(ctx, value(768'000'000));

    do_spacer(ctx, height(20, PIXELS));

    //     // Create a tree-sitter parser for C++
    //     auto parser = ts_parser_new();
    //     ts_parser_set_language(parser, tree_sitter_cpp());

    //     // Parse the source code
    //     const char* source = R"(
    // void
    // factor_tree(ui_context ctx, readable<int> n)
    // {
    //     // Get the 'best' factor that n has. (The one closest to sqrt(n).)
    //     auto f = alia::apply(ctx, factor, n);

    //     // If that factor isn't 1, n is composite.
    //     if_(ctx, f != 1, [&] {
    //         auto show_factors = get_state(ctx, false);

    //         {
    //             row_layout row(ctx);
    //             do_node_expander(ctx, show_factors);
    //             do_text(
    //                 ctx, direct(my_style), alia::printf(ctx, "%i:
    //                 composite", n));
    //         }

    //         collapsible_content(ctx, show_factors, GROW, [&] {
    //             row_layout row(ctx);
    //             do_spacer(ctx, width(40, PIXELS));
    //             column_layout column(ctx, GROW);
    //             factor_tree(ctx, f);
    //             factor_tree(ctx, n / f);
    //         });
    //     }).else_([&] {
    //         row_layout row(ctx);
    //         do_spacer(ctx, width(40, PIXELS));
    //         do_text(ctx, direct(my_style), alia::printf(ctx, "%i: prime",
    //         n));
    //     });
    // }
    //     )";

    //     TSTree* tree = ts_parser_parse_string(
    //         parser, nullptr, source, static_cast<uint32_t>(strlen(source)));

    //     // Get the root node
    //     TSNode root = ts_tree_root_node(tree);

    //     // Function to recursively traverse and display the AST
    //     std::function<void(TSNode, int)> display_node = [&](TSNode node,
    //                                                         int depth) {
    //         // Create indentation
    //         std::string indent(depth * 2, ' ');

    //         // Get node text content
    //         uint32_t start_byte = ts_node_start_byte(node);
    //         uint32_t end_byte = ts_node_end_byte(node);
    //         std::string node_text(source + start_byte, end_byte -
    //         start_byte);

    //         // Display node with syntax highlighting
    //         alia_if (ts_node_is_named(node))
    //         {
    //             do_text(
    //                 ctx,
    //                 direct(my_style),
    //                 alia::printf(
    //                     ctx, "%s%s: %s", indent, ts_node_type(node),
    //                     node_text));
    //         }
    //         alia_else_if (!node_text.empty() && node_text[0] != '\n')
    //         {
    //             do_text(
    //                 ctx,
    //                 direct(my_style),
    //                 alia::printf(ctx, "%s'%s'", indent, node_text));
    //         }
    //         alia_end

    //         // Recursively display children
    //         uint32_t child_count = ts_node_child_count(node);
    //         alia_for(uint32_t i = 0; i < child_count; i++)
    //         {
    //             TSNode child = ts_node_child(node, i);
    //             display_node(child, depth + 1);
    //         }
    //         alia_end
    //     };

    //     // Start displaying from root
    //     display_node(root, 0);

    //     // Cleanup
    //     ts_tree_delete(tree);
    //     ts_parser_delete(parser);
}

void
introspection_browser(ui_context ctx, introspection_node* node)
{
    auto show_children = get_state(ctx, false);

    alia_if (node->first_child)
    {
        {
            row_layout row(ctx);
            do_node_expander(ctx, show_children);
            do_text(ctx, direct(my_style), value(node->name));
        }
        collapsible_content(ctx, show_children, GROW, [&] {
            row_layout row(ctx);
            do_spacer(ctx, width(40, PIXELS));
            column_layout column(ctx, GROW);
            alia_for(auto* child = node->first_child; child;
                     child = child->next_sibling)
            {
                introspection_browser(ctx, child);
            }
            alia_end
        });
    }
    alia_else
    {
        row_layout row(ctx);
        do_spacer(ctx, width(40, PIXELS));
        do_text(ctx, direct(my_style), value(node->name));
    }
    alia_end
}

void
my_ui(ui_context ctx)
{
    introspection_traversal traversal;
    traversal.next_ptr = &root_node.first_child;
    the_traversal = &traversal;

    row_layout row(ctx);

    app_ui(ctx);

    {
        scoped_scrollable_view scrollable(ctx, GROW);
        column_layout column(ctx, GROW | PADDED);
        do_text(ctx, direct(my_style), value("Introspection"));
        introspection_browser(ctx, &root_node);
    }
}
