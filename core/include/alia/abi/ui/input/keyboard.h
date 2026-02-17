#ifndef ALIA_ABI_UI_INPUT_KEYBOARD_H
#define ALIA_ABI_UI_INPUT_KEYBOARD_H

#include <alia/abi/context.h>
#include <alia/abi/ids.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/constants.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_modded_key
{
    alia_key_code_t code;
    alia_kmods_t mods;
} alia_modded_key;

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
    alia_context* ctx, alia_element_id id, alia_modded_key* out);
bool
alia_element_detect_key_release(
    alia_context* ctx, alia_element_id id, alia_modded_key* out);

// TODO: Implement text input.
// bool
// alia_element_detect_text_input(
//     alia_context* ctx,
//     alia_element_id id,
//     char* buf,
//     size_t buf_size,
//     size_t* out_len);

// key events (background)
bool
alia_input_detect_key_press(alia_context* ctx, alia_modded_key* out);
bool
alia_input_detect_key_release(alia_context* ctx, alia_modded_key* out);

// keyboard "clicking"

typedef struct alia_keyboard_click_state
{
    int state;
} alia_keyboard_click_state;

bool
alia_element_detect_keyboard_click(
    alia_context* ctx,
    alia_keyboard_click_state* state,
    alia_element_id id,
    alia_key_code_t code,
    alia_kmods_t mods);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_INPUT_KEYBOARD_H */
