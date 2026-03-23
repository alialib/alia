#ifndef ALIA_ABI_EVENTS_H
#define ALIA_ABI_EVENTS_H

#include <alia/abi/base/arena.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/ids.h>
#include <alia/abi/kernel/events.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/layout/protocol.h>

#include <string.h>

ALIA_EXTERN_C_BEGIN

// CATEGORIES

enum
{
    ALIA_CATEGORY_DRAWING = 2,
    ALIA_CATEGORY_SPATIAL = 3,
    ALIA_CATEGORY_INPUT = 4,
};

enum
{
    ALIA_EVENT_FLAGS_NONE = 0,
    ALIA_EVENT_FLAGS_TARGETED = 1 << 0,
};

// EVENT TYPES
//
// The core event types are defined via a macro table that enumerates all the
// event types.
//
// The columns are:
// `(code, category, flags, uppercase_name, lowercase_name, data_struct_name)`
//
#define ALIA_EVENTS(X)                                                        \
    /* none / meta */                                                         \
    X(0x00, NONE, NONE, NONE, none, alia_nil)                                 \
    /* refresh */                                                             \
    X(0x10, REFRESH, NONE, REFRESH, refresh, alia_refresh)                    \
    /* drawing */                                                             \
    X(0x20, DRAWING, NONE, DRAW, draw, alia_draw)                             \
    /* spatial (geometry-aware routing) */                                    \
    X(0x30,                                                                   \
      SPATIAL,                                                                \
      TARGETED,                                                               \
      MAKE_WIDGET_VISIBLE,                                                    \
      make_widget_visible,                                                    \
      alia_make_widget_visible)                                               \
    X(0x31,                                                                   \
      SPATIAL,                                                                \
      NONE,                                                                   \
      MOUSE_HIT_TEST,                                                         \
      mouse_hit_test,                                                         \
      alia_mouse_hit_test)                                                    \
    X(0x32,                                                                   \
      SPATIAL,                                                                \
      NONE,                                                                   \
      WHEEL_HIT_TEST,                                                         \
      wheel_hit_test,                                                         \
      alia_wheel_hit_test)                                                    \
    /* keyboard */                                                            \
    X(0x40, INPUT, TARGETED, TEXT_INPUT, text_input, alia_text_input)         \
    X(0x41,                                                                   \
      INPUT,                                                                  \
      NONE,                                                                   \
      BACKGROUND_TEXT_INPUT,                                                  \
      background_text_input,                                                  \
      alia_text_input)                                                        \
    X(0x42, INPUT, TARGETED, KEY_PRESS, key_press, alia_key_input)            \
    X(0x43,                                                                   \
      INPUT,                                                                  \
      NONE,                                                                   \
      BACKGROUND_KEY_PRESS,                                                   \
      background_key_press,                                                   \
      alia_key_input)                                                         \
    X(0x44, INPUT, TARGETED, KEY_RELEASE, key_release, alia_key_input)        \
    X(0x45,                                                                   \
      INPUT,                                                                  \
      NONE,                                                                   \
      BACKGROUND_KEY_RELEASE,                                                 \
      background_key_release,                                                 \
      alia_key_input)                                                         \
    /* focus */                                                               \
    X(0x50, INPUT, TARGETED, FOCUS_GAIN, focus_gain, alia_focus_notification) \
    X(0x51, INPUT, TARGETED, FOCUS_LOSS, focus_loss, alia_focus_notification) \
    X(0x52,                                                                   \
      INPUT,                                                                  \
      NONE,                                                                   \
      FOCUS_PREDECESSOR,                                                      \
      focus_predecessor,                                                      \
      alia_focus_predecessor)                                                 \
    X(0x53,                                                                   \
      INPUT,                                                                  \
      NONE,                                                                   \
      FOCUS_SUCCESSOR,                                                        \
      focus_successor,                                                        \
      alia_focus_successor)                                                   \
    X(0x54, INPUT, NONE, FOCUS_RECOVERY, focus_recovery, alia_focus_recovery) \
    /* mouse / pointer */                                                     \
    X(0x60, INPUT, TARGETED, MOUSE_PRESS, mouse_press, alia_mouse_button)     \
    X(0x61, INPUT, TARGETED, DOUBLE_CLICK, double_click, alia_mouse_button)   \
    X(0x62, INPUT, TARGETED, MOUSE_RELEASE, mouse_release, alia_mouse_button) \
    X(0x63, INPUT, TARGETED, MOUSE_MOTION, mouse_motion, alia_mouse_motion)   \
    X(0x64, INPUT, TARGETED, MOUSE_GAIN, mouse_gain, alia_mouse_notification) \
    X(0x65, INPUT, TARGETED, MOUSE_LOSS, mouse_loss, alia_mouse_notification) \
    X(0x66,                                                                   \
      INPUT,                                                                  \
      TARGETED,                                                               \
      MOUSE_HOVER,                                                            \
      mouse_hover,                                                            \
      alia_mouse_notification)                                                \
    X(0x67, INPUT, TARGETED, CURSOR_QUERY, cursor_query, alia_cursor_query)   \
    /* scroll */                                                              \
    X(0x70, INPUT, TARGETED, WHEEL, wheel, alia_wheel)                        \
    /* timer */                                                               \
    X(0x82, INPUT, TARGETED, TIMER, timer, alia_timer)
/* misc */
//
// X(0x83, NONE, RESOLVE_LOCATION, TARGETED, resolve_location,
// alia_resolve_location)
//
// X(0x84, NONE, SHUTDOWN, NONE, shutdown, alia_nil)
//
// X(0x85, NONE, CUSTOM, NONE, custom, alia_custom_event)

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
    alia_routable_element_id id;
    alia_cursor_t cursor;
} alia_mouse_hit_test_result;

typedef struct alia_mouse_hit_test
{
    float x;
    float y;
    alia_mouse_hit_test_result result;
} alia_mouse_hit_test;

typedef struct alia_wheel_hit_test
{
    float x;
    float y;
    alia_routable_element_id result;
} alia_wheel_hit_test;

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
    alia_routable_element_id successor;
    bool just_saw_target;
} alia_focus_successor;

typedef struct alia_focus_predecessor
{
    alia_element_id target;
    alia_routable_element_id predecessor;
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

typedef struct alia_wheel
{
    alia_vec2f delta;
} alia_wheel;

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
