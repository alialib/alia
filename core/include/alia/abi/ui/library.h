#ifndef ALIA_ABI_UI_LIBRARY_H
#define ALIA_ABI_UI_LIBRARY_H

#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/palette.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_switch_style
{
    uint8_t off_track;
    uint8_t on_track;
    uint8_t off_dot;
    uint8_t on_dot;
    uint8_t highlight;
    uint8_t disabled_track;
    uint8_t disabled_dot;
} alia_switch_style;

alia_element_id
alia_do_switch(
    alia_context* ctx,
    bool* state, // TODO: Use `alia_signal_bool` instead.
    alia_layout_flags_t layout_flags,
    alia_switch_style const* style);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_LIBRARY_H */
