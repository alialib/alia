#ifndef ALIA_ABI_UI_LIBRARY_H
#define ALIA_ABI_UI_LIBRARY_H

#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/layout/flags.h>

ALIA_EXTERN_C_BEGIN

alia_element_id
alia_do_switch(
    alia_context* ctx,
    bool* state, // TODO: Use `alia_signal_bool` instead.
    alia_layout_flags_t layout_flags);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_LIBRARY_H */
