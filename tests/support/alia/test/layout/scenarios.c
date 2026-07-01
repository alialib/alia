#include <alia/test/layout/scenarios.h>

#include <alia/abi/base/geometry/vec2.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/layout/flags.h>

static void
scenario_leaf(
    alia_context* ctx, float width, float height, alia_layout_flags_t flags)
{
    if (alia_layout_context_is_refresh(ctx))
    {
        alia_layout_leaf_emit(
            ctx,
            alia_layout_content_metrics_make(alia_vec2f_make(width, height)),
            flags);
    }
    else
        (void) alia_layout_consume_box(ctx);
}

void
alia_layout_scenario_nested_grid_10(alia_context* ctx, void* user_data)
{
    (void) user_data;

    alia_layout_row_begin(ctx, 0, 0);
    for (uint32_t i = 0; i < 10; ++i)
    {
        alia_layout_column_begin(ctx, 0, 0);
        for (uint32_t j = 0; j < 10; ++j)
        {
            alia_layout_row_begin(ctx, 0, 0);
            for (uint32_t k = 0; k < 10; ++k)
                scenario_leaf(ctx, 100.f, 100.f, 0);
            alia_layout_row_end(ctx);
        }
        alia_layout_column_end(ctx);
    }
    alia_layout_row_end(ctx);
}

void
alia_layout_scenario_column_of_rows_100(alia_context* ctx, void* user_data)
{
    (void) user_data;

    alia_layout_column_begin(ctx, 0, 0);
    for (uint32_t j = 0; j < 100; ++j)
    {
        alia_layout_row_begin(ctx, 0, 0);
        scenario_leaf(ctx, 20.f, 20.f, 0);
        scenario_leaf(ctx, 20.f, (float) ((j & 7u) * 5u), 0);
        alia_layout_row_end(ctx);
    }
    alia_layout_column_end(ctx);
}

void
alia_layout_scenario_growth_rows_100(alia_context* ctx, void* user_data)
{
    (void) user_data;

    alia_layout_column_begin(ctx, 0, 0);
    for (uint32_t j = 0; j < 100; ++j)
    {
        alia_layout_row_begin(ctx, ALIA_GROW, 0);
        scenario_leaf(ctx, 20.f, 20.f, 0);
        scenario_leaf(ctx, 0.f, 0.f, ALIA_FILL | ALIA_GROW);
        scenario_leaf(ctx, (float) ((j & 7u) * 5u), 0.f, 0);
        alia_layout_row_end(ctx);
    }
    alia_layout_column_end(ctx);
}

uint32_t
alia_layout_scenario_nested_grid_10_leaf_count(void)
{
    return 10u * 10u * 10u;
}

uint32_t
alia_layout_scenario_column_of_rows_100_leaf_count(void)
{
    return 100u * 2u;
}

uint32_t
alia_layout_scenario_growth_rows_100_leaf_count(void)
{
    return 100u * 3u;
}
