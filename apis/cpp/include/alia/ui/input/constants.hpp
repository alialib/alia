#pragma once

#include <alia/abi/ui/input/constants.h>
#include <alia/flags.hpp>

namespace alia {

// MOUSE/POINTER BUTTONS

static constexpr unsigned max_supported_buttons = ALIA_MAX_SUPPORTED_BUTTONS;

#define X(code, NAME, name) name = (code),
enum class button : alia_button_t
{
    ALIA_BUTTONS(X)
};
#undef X

// CURSORS

enum class cursor : alia_cursor_t
{
#define X(name) name = ALIA_CURSOR_##name,
    ALIA_CURSORS(X)
#undef X
};

// KEYS

// keyboard modifier keys
ALIA_DEFINE_FLAG_TYPE(alia_kmods_t, kmods)
#define X(code, NAME, name) ALIA_DEFINE_FLAG(kmods, code, KMOD_##NAME);
ALIA_KEY_MODS(X)
#undef X

struct modded_key
{
    alia_key_code_t code;
    kmods_flag_set mods;
};

} // namespace alia
