#include <alia/ui/styling/descriptors_internal.h>

#include <alia/abi/ui/styling/descriptors.h>
#include <alia/abi/ui/styling/environment.h>
#include <alia/abi/ui/styling/palette_generation.h>
#include <alia/ui/system/object.h>

#include <cstring>

namespace {

alia_style_float_desc const core_var_descs[] = {
#define ALIA_STYLE_VAR_DESC_X(NAME, DESC, GROUP, MIN, MAX, STEP, DL, DD)      \
    {{#NAME, DESC, ALIA_STYLE_GROUP_##GROUP},                                 \
     (uint16_t) ALIA_STYLE_VAR_##NAME,                                        \
     MIN,                                                                     \
     MAX,                                                                     \
     STEP,                                                                    \
     DL,                                                                      \
     DD},
    ALIA_STYLE_VARS(ALIA_STYLE_VAR_DESC_X)
#undef ALIA_STYLE_VAR_DESC_X
};

alia_style_float_desc const seed_descs[] = {
#define ALIA_STYLE_SEED_DESC_X(NAME, DESC, GROUP, MIN, MAX, STEP, DL, DD)     \
    {{#NAME, DESC, ALIA_STYLE_GROUP_##GROUP},                                 \
     (uint16_t) (offsetof(alia_style_seeds, NAME) / sizeof(float)),           \
     MIN,                                                                     \
     MAX,                                                                     \
     STEP,                                                                    \
     DL,                                                                      \
     DD},
    ALIA_STYLE_SEEDS(ALIA_STYLE_SEED_DESC_X)
#undef ALIA_STYLE_SEED_DESC_X
};

alia_style_float_desc const recipe_descs[] = {
#define ALIA_STYLE_RECIPE_DESC_X(NAME, DESC, GROUP, MIN, MAX, STEP, DL, DD)   \
    {{#NAME, DESC, ALIA_STYLE_GROUP_##GROUP},                                 \
     (uint16_t) (offsetof(alia_palette_recipe, NAME) / sizeof(float)),        \
     MIN,                                                                     \
     MAX,                                                                     \
     STEP,                                                                    \
     DL,                                                                      \
     DD},
    ALIA_PALETTE_RECIPE_FIELDS(ALIA_STYLE_RECIPE_DESC_X)
#undef ALIA_STYLE_RECIPE_DESC_X
};

alia_style_color_desc const core_color_descs[] = {
#define ALIA_STYLE_COLOR_DESC_X(NAME, DESC, GROUP)                            \
    {{#NAME, DESC, ALIA_STYLE_GROUP_##GROUP},                                 \
     (uint8_t) ALIA_STYLE_COLOR_##NAME},
    ALIA_STYLE_COLORS(ALIA_STYLE_COLOR_DESC_X)
#undef ALIA_STYLE_COLOR_DESC_X
};

bool
var_slot_registered(alia::style_system const& styling, uint16_t index)
{
    for (size_t i = 0; i < styling.var_desc_count; ++i)
    {
        if (styling.var_descs[i].index == index)
            return true;
    }
    return false;
}

bool
color_slot_registered(alia::style_system const& styling, uint8_t slot)
{
    for (size_t i = 0; i < styling.color_desc_count; ++i)
    {
        if (styling.color_descs[i].slot == slot)
            return true;
    }
    return false;
}

} // namespace

namespace {

void
apply_var_descs(
    alia_style_environment* out,
    bool dark,
    alia_style_float_desc const* descs,
    size_t count)
{
    ALIA_ASSERT(out);
    for (int i = 0; i < ALIA_STYLE_VAR_COUNT; ++i)
        out->vars[i] = 0.f;
    for (size_t i = 0; i < count; ++i)
    {
        alia_style_float_desc const& d = descs[i];
        if (d.index >= ALIA_STYLE_VAR_COUNT)
            continue;
        out->vars[d.index] = dark ? d.default_dark : d.default_light;
    }
}

} // namespace

namespace alia {

void
style_vars_apply_core_defaults(alia_style_environment* out, bool dark)
{
    apply_var_descs(
        out,
        dark,
        core_var_descs,
        sizeof(core_var_descs) / sizeof(core_var_descs[0]));
}

void
style_descriptors_init(style_system& styling, bool dark)
{
    static_assert(
        sizeof(core_var_descs) / sizeof(core_var_descs[0])
            == ALIA_STYLE_VAR_CORE_COUNT,
        "core var desc count");
    static_assert(
        sizeof(core_color_descs) / sizeof(core_color_descs[0])
            == ALIA_STYLE_COLOR_CORE_COUNT,
        "core color desc count");

    std::memcpy(
        styling.var_descs, core_var_descs, sizeof(core_var_descs));
    styling.var_desc_count = ALIA_STYLE_VAR_CORE_COUNT;

    std::memcpy(
        styling.color_descs, core_color_descs, sizeof(core_color_descs));
    styling.color_desc_count = ALIA_STYLE_COLOR_CORE_COUNT;

    apply_var_descs(
        &styling.environment,
        dark,
        styling.var_descs,
        styling.var_desc_count);
}

} // namespace alia

extern "C" {

alia_style_float_desc const*
alia_style_var_descs(alia_ui_system* ui, size_t* count)
{
    ALIA_ASSERT(ui);
    if (count)
        *count = ui->styling.var_desc_count;
    return ui->styling.var_descs;
}

alia_style_float_desc const*
alia_style_seed_descs(size_t* count)
{
    if (count)
        *count = sizeof(seed_descs) / sizeof(seed_descs[0]);
    return seed_descs;
}

alia_style_float_desc const*
alia_style_recipe_descs(size_t* count)
{
    if (count)
        *count = sizeof(recipe_descs) / sizeof(recipe_descs[0]);
    return recipe_descs;
}

alia_style_color_desc const*
alia_style_color_descs(alia_ui_system* ui, size_t* count)
{
    ALIA_ASSERT(ui);
    if (count)
        *count = ui->styling.color_desc_count;
    return ui->styling.color_descs;
}

int
alia_style_var_register(alia_ui_system* ui, alia_style_float_desc const* desc)
{
    if (!ui || !desc || !desc->meta.name)
        return -1;
    if (desc->index < ALIA_STYLE_VAR_APP_BEGIN
        || desc->index >= ALIA_STYLE_VAR_COUNT)
        return -1;
    if (var_slot_registered(ui->styling, desc->index))
        return -1;
    if (ui->styling.var_desc_count >= ALIA_STYLE_VAR_COUNT)
        return -1;
    ui->styling.var_descs[ui->styling.var_desc_count++] = *desc;
    return 0;
}

int
alia_style_color_register(
    alia_ui_system* ui, alia_style_color_desc const* desc)
{
    if (!ui || !desc || !desc->meta.name)
        return -1;
    if (desc->slot < ALIA_STYLE_COLOR_APP_BEGIN
        || desc->slot >= ALIA_STYLE_COLOR_COUNT)
        return -1;
    if (color_slot_registered(ui->styling, desc->slot))
        return -1;
    if (ui->styling.color_desc_count >= ALIA_STYLE_COLOR_COUNT)
        return -1;
    ui->styling.color_descs[ui->styling.color_desc_count++] = *desc;
    return 0;
}

} // extern "C"
