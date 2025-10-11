
#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include <vector>

#include <alia/kernel/infinite_arena.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/layout/compositors/column.hpp>
#include <alia/ui/layout/compositors/row.hpp>
#include <alia/ui/layout/container.hpp>
#include <alia/ui/layout/leaf.hpp>
#include <alia/ui/system/object.hpp>

using namespace alia;

template<class Content>
void
add_content(System& system, Content&& content)
{
    Style style = {.padding = 0};

    system.layout.node_arena.reset();
    Context refresh_ctx = {
        Pass{
            PassType::Refresh,
            {.refresh
             = {.layout_emission
                = {&system.layout.node_arena,
                   &system.layout.root,
                   &system.layout.root.first_child}}}},
        &style,
        &system};

    content(refresh_ctx);

    *refresh_ctx.pass.refresh.layout_emission.next_ptr = 0;
}

void
do_leaf(Context& ctx, Vec2 size, LayoutFlagSet flags = NO_FLAGS)
{
    if (ctx.pass.type != PassType::Refresh)
    {
        auto& layout = ctx.pass.refresh.layout_emission;
        LayoutLeafNode* new_node = arena_alloc<LayoutLeafNode>(
            *ctx.pass.refresh.layout_emission.arena);
        *layout.next_ptr = &new_node->base;
        layout.next_ptr = &new_node->base.next_sibling;
        *new_node = LayoutLeafNode{
            .base = {.vtable = &leaf_vtable, .next_sibling = 0},
            .flags = flags,
            .padding = ctx.style->padding,
            .size = size};
    }
}

int
main()
{
    System system;
    initialize(system, Vec2{10'000, 10'000}, Vec2{1.0f, 1.0f});

    add_content(system, [&](auto& ctx) {
        row(ctx, [&] {
            for (uint32_t i = 0; i < 10; ++i)
            {
                column(ctx, [&] {
                    for (uint32_t i = 0; i < 10; ++i)
                    {
                        row(ctx, [&] {
                            for (uint32_t i = 0; i < 10; ++i)
                            {
                                do_leaf(ctx, Vec2(100, 100));
                            }
                        });
                    }
                });
            }
        });
    });

    ankerl::nanobench::Bench().minEpochIterations(1000).run(
        "add_content", [&] {
            add_content(system, [&](auto& ctx) {
                row(ctx, [&] {
                    for (uint32_t i = 0; i < 10; ++i)
                    {
                        column(ctx, [&] {
                            for (uint32_t i = 0; i < 10; ++i)
                            {
                                row(ctx, [&] {
                                    for (uint32_t i = 0; i < 10; ++i)
                                    {
                                        do_leaf(ctx, Vec2(100, 100));
                                    }
                                });
                            }
                        });
                    }
                });
            });
            ankerl::nanobench::doNotOptimizeAway(
                system.layout.root.first_child);
        });

    ankerl::nanobench::Bench().minEpochIterations(1000).run(
        "resolve_layout", [&] {
            resolve_layout(system.layout, Vec2{10'000, 10'000});
            ankerl::nanobench::doNotOptimizeAway(
                system.layout.placement_arena.peek());
        });
}
