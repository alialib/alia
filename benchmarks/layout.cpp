
#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include <alia/foundation/infinite_arena.hpp>
#include <alia/ui/layout/resolution.hpp>

using namespace alia;

struct LayoutBenchmarkSystem
{
    LayoutScratchArena scratch;
    std::vector<LayoutSpec> specs;
    std::vector<LayoutPlacement> placements;
};

struct LayoutBenchmarkContext
{
    std::vector<LayoutSpec>* specs;
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

    system.specs.clear();
    // Add a placeholder to fill the reserved/invalid 0 index.
    system.specs.push_back(LayoutSpec{});

    std::uint32_t root_index;
    LayoutBenchmarkContext ctx;
    ctx.specs = &system.specs;
    ctx.next = &root_index;

    content(ctx);

    *ctx.next = 0;

    system.placements.resize(system.specs.size());
}

void
add_leaf(LayoutBenchmarkContext& ctx, LayoutSpec spec)
{
    *ctx.next = static_cast<std::uint32_t>(ctx.specs->size());
    spec.type = LayoutNodeType::Leaf;
    ctx.specs->push_back(spec);
    ++(*ctx.specs)[ctx.active_container].child_count;
    ctx.next = &ctx.specs->back().next_sibling;
}

template<class Content>
void
add_hbox(LayoutBenchmarkContext& ctx, LayoutSpec spec, Content&& content)
{
    std::uint32_t this_index = static_cast<std::uint32_t>(ctx.specs->size());
    *ctx.next = this_index;

    spec.type = LayoutNodeType::HBox;
    ctx.specs->push_back(spec);

    std::uint32_t parent_container = ctx.active_container;
    ++(*ctx.specs)[parent_container].child_count;

    ctx.next = &(*ctx.specs)[this_index].first_child;
    ctx.active_container = this_index;

    content();

    *ctx.next = 0;
    ctx.next = &(*ctx.specs)[this_index].next_sibling;

    ctx.active_container = parent_container;
}

template<class Content>
void
add_vbox(LayoutBenchmarkContext& ctx, LayoutSpec spec, Content&& content)
{
    std::uint32_t this_index = static_cast<std::uint32_t>(ctx.specs->size());
    *ctx.next = this_index;

    spec.type = LayoutNodeType::VBox;
    ctx.specs->push_back(spec);

    std::uint32_t parent_container = ctx.active_container;
    ++(*ctx.specs)[parent_container].child_count;

    ctx.next = &(*ctx.specs)[this_index].first_child;
    ctx.active_container = this_index;

    content();

    *ctx.next = 0;
    ctx.next = &(*ctx.specs)[this_index].next_sibling;

    ctx.active_container = parent_container;
}

int
main()
{
    LayoutBenchmarkSystem system;
    initialize(system);

    add_content(system, [&](auto& ctx) {
        add_hbox(ctx, LayoutSpec{}, [&] {
            for (uint32_t i = 0; i < 10; ++i)
            {
                add_vbox(ctx, LayoutSpec{}, [&] {
                    for (uint32_t i = 0; i < 10; ++i)
                    {
                        add_hbox(ctx, LayoutSpec{}, [&] {
                            for (uint32_t i = 0; i < 10; ++i)
                            {
                                add_leaf(
                                    ctx, LayoutSpec{.size = Vec2(100, 100)});
                            }
                        });
                    }
                });
            }
        });
    });

    ankerl::nanobench::Bench().run("resolve_layout", [&] {
        resolve_layout(
            system.specs.data(),
            system.scratch,
            system.placements.data(),
            Vec2{10'000, 1000});
        ankerl::nanobench::doNotOptimizeAway(system.placements);
    });
}
