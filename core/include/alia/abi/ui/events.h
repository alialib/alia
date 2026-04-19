#ifndef ALIA_ABI_EVENTS_H
#define ALIA_ABI_EVENTS_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/events.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/layout/protocol.h>

#include <string.h>

ALIA_EXTERN_C_BEGIN

// CATEGORIES

enum
{
    ALIA_CATEGORY_DRAWING = 3,
    ALIA_CATEGORY_SPATIAL = 4,
};

enum
{
    ALIA_EVENT_FLAGS_NONE = 0,
    ALIA_EVENT_FLAGS_TARGETED = 1 << 0,
};

// EVENT TYPES - This extends the kervel event table and follows the same
// format.
#define ALIA_EVENTS(X)                                                        \
    ALIA_KERNEL_EVENTS(X)                                                     \
    /* drawing */                                                             \
    X(0x30, DRAWING, NONE, DRAW, draw, alia_draw)                             \
    /* spatial (geometry-aware routing) */                                    \
    X(0x40,                                                                   \
      SPATIAL,                                                                \
      TARGETED,                                                               \
      MAKE_WIDGET_VISIBLE,                                                    \
      make_widget_visible,                                                    \
      alia_make_widget_visible)                                               \
    X(0x41,                                                                   \
      SPATIAL,                                                                \
      NONE,                                                                   \
      MOUSE_HIT_TEST,                                                         \
      mouse_hit_test,                                                         \
      alia_mouse_hit_test)                                                    \
    X(0x42,                                                                   \
      SPATIAL,                                                                \
      NONE,                                                                   \
      SCROLL_INPUT_HIT_TEST,                                                  \
      scroll_input_hit_test,                                                  \
      alia_scroll_input_hit_test)                                             \
    /* X(0x83, SPATIAL, TARGETED, RESOLVE_LOCATION, resolve_location,         \
        alia_resolve_location) */                                             \
    /* keyboard */                                                            \
    X(0x50, INPUT, TARGETED, TEXT_INPUT, text_input, alia_text_input)         \
    X(0x51,                                                                   \
      INPUT,                                                                  \
      NONE,                                                                   \
      BACKGROUND_TEXT_INPUT,                                                  \
      background_text_input,                                                  \
      alia_text_input)                                                        \
    X(0x52, INPUT, TARGETED, KEY_PRESS, key_press, alia_key_input)            \
    X(0x53,                                                                   \
      INPUT,                                                                  \
      NONE,                                                                   \
      BACKGROUND_KEY_PRESS,                                                   \
      background_key_press,                                                   \
      alia_key_input)                                                         \
    X(0x54, INPUT, TARGETED, KEY_RELEASE, key_release, alia_key_input)        \
    X(0x55,                                                                   \
      INPUT,                                                                  \
      NONE,                                                                   \
      BACKGROUND_KEY_RELEASE,                                                 \
      background_key_release,                                                 \
      alia_key_input)                                                         \
    /* focus */                                                               \
    X(0x60, INPUT, TARGETED, FOCUS_GAIN, focus_gain, alia_focus_notification) \
    X(0x61, INPUT, TARGETED, FOCUS_LOSS, focus_loss, alia_focus_notification) \
    X(0x62,                                                                   \
      INPUT,                                                                  \
      NONE,                                                                   \
      FOCUS_PREDECESSOR,                                                      \
      focus_predecessor,                                                      \
      alia_focus_predecessor)                                                 \
    X(0x63,                                                                   \
      INPUT,                                                                  \
      NONE,                                                                   \
      FOCUS_SUCCESSOR,                                                        \
      focus_successor,                                                        \
      alia_focus_successor)                                                   \
    X(0x64, INPUT, NONE, FOCUS_RECOVERY, focus_recovery, alia_focus_recovery) \
    /* mouse / pointer */                                                     \
    X(0x70, INPUT, TARGETED, MOUSE_PRESS, mouse_press, alia_mouse_button)     \
    X(0x71, INPUT, TARGETED, DOUBLE_CLICK, double_click, alia_mouse_button)   \
    X(0x72, INPUT, TARGETED, MOUSE_RELEASE, mouse_release, alia_mouse_button) \
    X(0x73, INPUT, TARGETED, MOUSE_MOTION, mouse_motion, alia_mouse_motion)   \
    X(0x74, INPUT, TARGETED, MOUSE_GAIN, mouse_gain, alia_mouse_notification) \
    X(0x75, INPUT, TARGETED, MOUSE_LOSS, mouse_loss, alia_mouse_notification) \
    X(0x76,                                                                   \
      INPUT,                                                                  \
      TARGETED,                                                               \
      MOUSE_HOVER,                                                            \
      mouse_hover,                                                            \
      alia_mouse_notification)                                                \
    X(0x77, INPUT, TARGETED, CURSOR_QUERY, cursor_query, alia_cursor_query)   \
    /* scroll */                                                              \
    X(0x80, INPUT, TARGETED, SCROLL_INPUT, scroll_input, alia_scroll_input)

// These are the event-specific data structures that correspond to the event
// types in the table above...

typedef struct alia_refresh
{
    bool incomplete;
} alia_refresh;

typedef struct alia_draw_context alia_draw_context;

typedef struct alia_draw
{
    // TODO: Not this.
    alia_draw_context* context;
} alia_draw;

typedef struct alia_nil
{
    // TODO
    int dummy;
} alia_nil;

typedef struct alia_make_widget_visible
{
    bool acknowledged;
    alia_box region;
} alia_make_widget_visible;

typedef struct alia_mouse_hit_test_result
{
    alia_element_id id;
    alia_cursor_t cursor;
} alia_mouse_hit_test_result;

typedef struct alia_mouse_hit_test
{
    float x;
    float y;
    alia_mouse_hit_test_result result;
} alia_mouse_hit_test;

typedef struct alia_scroll_input_hit_test
{
    float x;
    float y;
    alia_element_id result;
} alia_scroll_input_hit_test;

typedef struct alia_cursor_query
{
    alia_cursor_t cursor;
} alia_cursor_query;

typedef struct alia_text_input
{
    char* text;
    size_t text_length;
    bool acknowledged;
} alia_text_input;

typedef struct alia_key_input
{
    alia_key_code_t code;
    alia_kmods_t mods;
    bool acknowledged;
} alia_key_input;

typedef struct alia_focus_notification
{
    alia_element_id target;
} alia_focus_notification;

typedef struct alia_focus_recovery
{
    alia_element_id target;
} alia_focus_recovery;

typedef struct alia_focus_successor
{
    alia_element_id target;
    alia_element_id successor;
    bool just_saw_target;
} alia_focus_successor;

typedef struct alia_focus_predecessor
{
    alia_element_id target;
    alia_element_id predecessor;
    bool saw_target;
} alia_focus_predecessor;

typedef struct alia_mouse_button
{
    alia_button_t button;
    alia_kmods_t mods;
    float x;
    float y;
} alia_mouse_button;

typedef struct alia_mouse_motion
{
    float x;
    float y;
    float last_x;
    float last_y;
} alia_mouse_motion;

typedef struct alia_scroll_input
{
    alia_vec2f delta;
} alia_scroll_input;

typedef struct alia_mouse_notification
{
    int dummy;
} alia_mouse_notification;

typedef struct alia_timer
{
    alia_element_id target;
    alia_nanosecond_count fire_time;
} alia_timer;

// EVENT CODES

// clang-format off
enum
{
    #define X(code, CATEGORY, flags, NAME, name, data_type)                       \
        ALIA_EVENT_##NAME = (code),
    ALIA_EVENTS(X)
    #undef X
};
// clang-format on

// CONSTRUCTORS

#define X(code, CATEGORY, flags, NAME, name, data_type)                       \
    static inline alia_event alia_make_##name##_event(data_type data)         \
    {                                                                         \
        ALIA_ASSERT(sizeof(data) <= ALIA_EVENT_PAYLOAD_SIZE_MAX);             \
        alia_event event;                                                     \
        event.category = ALIA_CATEGORY_##CATEGORY;                            \
        event.type = ALIA_EVENT_##NAME;                                       \
        event.target = ALIA_ELEMENT_ID_NONE;                                  \
        event.payload_size = sizeof(data);                                    \
        memcpy(event.payload, &data, sizeof(data));                           \
        return event;                                                         \
    }

ALIA_EVENTS(X)
#undef X

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_EVENTS_H */
