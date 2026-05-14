#ifndef ALIA_ABI_UI_INPUT_KEYBOARD_H
#define ALIA_ABI_UI_INPUT_KEYBOARD_H

#include <alia/abi/context.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/constants.h>

ALIA_EXTERN_C_BEGIN

void
alia_input_acknowledge_key_event(alia_context* ctx);

// focus
void
alia_element_add_to_focus_order(alia_context* ctx, alia_element_id id);
bool
alia_element_has_focus(alia_context* ctx, alia_element_id id);
bool
alia_element_detect_focus_gain(alia_context* ctx, alia_element_id id);
bool
alia_element_detect_focus_loss(alia_context* ctx, alia_element_id id);
void
alia_element_focus_on_click(alia_context* ctx, alia_element_id id);

// key events (element-targeted)
bool
alia_element_detect_key_press(
    alia_context* ctx, alia_element_id id, alia_key_info* out);
bool
alia_element_detect_key_release(
    alia_context* ctx, alia_element_id id, alia_key_info* out);

// TODO: Implement text input.
// bool
// alia_element_detect_text_input(
//     alia_context* ctx,
//     alia_element_id id,
//     char* buf,
//     size_t buf_size,
//     size_t* out_len);

bool
alia_input_detect_key_press(alia_context* ctx, alia_key_info* out);
bool
alia_input_detect_key_release(alia_context* ctx, alia_key_info* out);

// key events (global)
bool
alia_input_detect_global_key_press(alia_context* ctx, alia_key_info* out);
bool
alia_input_detect_global_key_release(alia_context* ctx, alia_key_info* out);

// keyboard "clicking"

typedef struct alia_keyboard_click_state
{
    int state;
} alia_keyboard_click_state;

// `spec` selects which key axes must match via `spec.fields_present` (see
// `alia_key_info_matches_spec` in constants.h). Modifiers must match
// `spec.mods`.
bool
alia_element_detect_keyboard_click(
    alia_context* ctx,
    alia_keyboard_click_state* state,
    alia_element_id id,
    alia_key_info spec);

static inline bool
alia_element_detect_space_bar_click(
    alia_context* ctx, alia_keyboard_click_state* state, alia_element_id id)
{
    return alia_element_detect_keyboard_click(
        ctx, state, id, alia_key_info_make_logical(ALIA_KEY_SPACE, 0));
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_INPUT_KEYBOARD_H */
