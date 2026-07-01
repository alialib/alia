#pragma once

#include <alia/abi/context.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef void (*alia_layout_scenario_fn)(alia_context* ctx, void* user_data);

bool
alia_layout_context_is_refresh(alia_context* ctx);

void
alia_layout_scenario_nested_grid_10(alia_context* ctx, void* user_data);

void
alia_layout_scenario_column_of_rows_100(alia_context* ctx, void* user_data);

void
alia_layout_scenario_growth_rows_100(alia_context* ctx, void* user_data);

uint32_t
alia_layout_scenario_nested_grid_10_leaf_count(void);

uint32_t
alia_layout_scenario_column_of_rows_100_leaf_count(void);

uint32_t
alia_layout_scenario_growth_rows_100_leaf_count(void);

ALIA_EXTERN_C_END
