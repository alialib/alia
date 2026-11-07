#ifndef ALIA_ABI_UI_STYLING_PAINT_H
#define ALIA_ABI_UI_STYLING_PAINT_H

#include <alia/abi/ui/styling/color_program.h>
#include <alia/abi/ui/styling/environment.h>

ALIA_EXTERN_C_BEGIN

// colors of a resolved box paint - The program path scales `border_width` to
// physical px via `px_scale`. `angle` stays in authoring degrees until the
// draw/packer boundary converts it.
typedef struct alia_resolved_box_paint
{
    alia_srgba8 fill;
    alia_srgba8 fill_end;
    alia_srgba8 border;
    alia_srgba8 border_end;
    bool gradient;
    float angle;
    float border_width;
} alia_resolved_box_paint;

// Resolve program paint for `status`. `steps` is the style's step base.
// `px_scale` converts logical border width to physical px.
static inline alia_resolved_box_paint
alia_responsive_box_paint_program_resolve(
    alia_style_environment const* env,
    alia_color_program_step const* steps,
    alia_responsive_box_paint_program const* paint,
    uint8_t status,
    float px_scale)
{
    alia_resolved_box_paint out;
    out.fill = alia_responsive_color_program_resolve(
        env, steps, &paint->fill, status);
    out.fill_end = alia_responsive_color_program_resolve(
        env, steps, &paint->fill_end, status);
    out.border = alia_responsive_color_program_resolve(
        env, steps, &paint->border, status);
    out.border_end = alia_responsive_color_program_resolve(
        env, steps, &paint->border_end, status);
    out.gradient = paint->gradient;
    out.angle = paint->angle;
    out.border_width = paint->border_width * px_scale;
    return out;
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_STYLING_PAINT_H
