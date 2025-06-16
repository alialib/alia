
#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include <alia/flow/infinite_arena.hpp>
#include <alia/ui/layout/resolution.hpp>

using namespace alia;

struct LayoutBenchmarkSystem
{
    LayoutScratchArena scratch;
    std::vector<LayoutNode> nodes;
    std::vector<LayoutPlacement> placements;
};

struct LayoutBenchmarkContext
{
    std::vector<LayoutNode>* nodes;
    std::uint32_t active_container = 0;
    std::uint32_t* next;
};

void
initialize(LayoutBenchmarkSystem& system)
{
    system.scratch.initialize();
}

template<class Content>
void
add_content(LayoutBenchmarkSystem& system, Content&& content)
{
    system.scratch.reset();

    system.nodes.clear();
    // Add a placeholder to fill the reserved/invalid 0 index.
    system.nodes.push_back(LayoutNode{});

    std::uint32_t root_index;
    LayoutBenchmarkContext ctx;
    ctx.nodes = &system.nodes;
    ctx.next = &root_index;

    content(ctx);

    *ctx.next = 0;

    system.placements.resize(system.nodes.size());
}

void
add_leaf(LayoutBenchmarkContext& ctx, LayoutNode node)
{
    *ctx.next = static_cast<std::uint32_t>(ctx.nodes->size());
    node.type = LayoutNodeType::Leaf;
    ctx.nodes->push_back(node);
    ++(*ctx.nodes)[ctx.active_container].child_count;
    ctx.next = &ctx.nodes->back().next_sibling;
}

template<class Content>
void
add_hbox(LayoutBenchmarkContext& ctx, LayoutNode node, Content&& content)
{
    std::uint32_t this_index = static_cast<std::uint32_t>(ctx.nodes->size());
    *ctx.next = this_index;

    node.type = LayoutNodeType::HBox;
    ctx.nodes->push_back(node);

    std::uint32_t parent_container = ctx.active_container;
    ++(*ctx.nodes)[parent_container].child_count;

    ctx.next = &(*ctx.nodes)[this_index].first_child;
    ctx.active_container = this_index;

    content();

    *ctx.next = 0;
    ctx.next = &(*ctx.nodes)[this_index].next_sibling;

    ctx.active_container = parent_container;
}

template<class Content>
void
add_vbox(LayoutBenchmarkContext& ctx, LayoutNode node, Content&& content)
{
    std::uint32_t this_index = static_cast<std::uint32_t>(ctx.nodes->size());
    *ctx.next = this_index;

    node.type = LayoutNodeType::VBox;
    ctx.nodes->push_back(node);

    std::uint32_t parent_container = ctx.active_container;
    ++(*ctx.nodes)[parent_container].child_count;

    ctx.next = &(*ctx.nodes)[this_index].first_child;
    ctx.active_container = this_index;

    content();

    *ctx.next = 0;
    ctx.next = &(*ctx.nodes)[this_index].next_sibling;

    ctx.active_container = parent_container;
}

int
main()
{
    LayoutBenchmarkSystem system;
    initialize(system);

    add_content(system, [&](auto& ctx) {
        add_hbox(ctx, LayoutNode{}, [&] {
            for (uint32_t i = 0; i < 10; ++i)
            {
                add_vbox(ctx, LayoutNode{}, [&] {
                    for (uint32_t i = 0; i < 10; ++i)
                    {
                        add_hbox(ctx, LayoutNode{}, [&] {
                            for (uint32_t i = 0; i < 10; ++i)
                            {
                                add_leaf(
                                    ctx, LayoutNode{.size = Vec2(100, 100)});
                            }
                        });
                    }
                });
            }
        });
    });

    ankerl::nanobench::Bench().run("resolve_layout", [&] {
        resolve_layout(
            system.nodes.data(),
            system.scratch,
            system.placements.data(),
            Vec2{10'000, 1000});
        ankerl::nanobench::doNotOptimizeAway(system.placements);
    });
}
