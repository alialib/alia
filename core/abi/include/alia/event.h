#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// CATEGORIES

typedef uint8_t alia_category_id;
typedef uint16_t alia_event_code;

enum
{
    ALIA_CATEGORY_NONE = 0,
    ALIA_CATEGORY_REFRESH = 1,
    ALIA_CATEGORY_DRAWING = 2,
    ALIA_CATEGORY_SPATIAL = 3,
    ALIA_CATEGORY_INPUT = 4,
};

// EVENT TYPES
//
// The core event types are defined via a macro table that enumerates all the
// event types.
//
// Calling `ALIA_ENUMERATE_EVENT_TYPES(X)` will invoke `X` as
// `X(code, category, uppercase_name, lowercase_name, data_struct_name)`
// for each built-in event type.
//
#define ALIA_ENUMERATE_EVENT_TYPES(X)                                         \
    /* none / meta */                                                         \
    /* refresh */                                                             \
    X(0x00, NONE, NONE, none, alia_nil)                                       \
    X(0x10, REFRESH, REFRESH, refresh, alia_refresh)                          \
    /* drawing */                                                             \
    X(0x20, DRAWING, RENDER, render, alia_render)                             \
    /* spatial (geometry-aware routing) */                                    \
    X(0x30,                                                                   \
      SPATIAL,                                                                \
      MAKE_WIDGET_VISIBLE,                                                    \
      make_widget_visible,                                                    \
      alia_make_widget_visible)                                               \
    X(0x31, SPATIAL, MOUSE_HIT_TEST, mouse_hit_test, alia_mouse_hit_test)     \
    X(0x32, SPATIAL, WHEEL_HIT_TEST, wheel_hit_test, alia_wheel_hit_test)     \
    X(0x33,                                                                   \
      SPATIAL,                                                                \
      MOUSE_CURSOR_QUERY,                                                     \
      mouse_cursor_query,                                                     \
      alia_mouse_cursor_query)                                                \
    /* keyboard */                                                            \
    X(0x40, INPUT, TEXT_INPUT, text_input, alia_text_input)                   \
    X(0x41,                                                                   \
      INPUT,                                                                  \
      BACKGROUND_TEXT_INPUT,                                                  \
      background_text_input,                                                  \
      alia_text_input)                                                        \
    X(0x42, INPUT, KEY_PRESS, key_press, alia_key_input)                      \
    X(0x43,                                                                   \
      INPUT,                                                                  \
      BACKGROUND_KEY_PRESS,                                                   \
      background_key_press,                                                   \
      alia_key_input)                                                         \
    X(0x44, INPUT, KEY_RELEASE, key_release, alia_key_input)                  \
    X(0x45,                                                                   \
      INPUT,                                                                  \
      BACKGROUND_KEY_RELEASE,                                                 \
      background_key_release,                                                 \
      alia_key_input)                                                         \
    /* focus */                                                               \
    X(0x50, INPUT, FOCUS_GAIN, focus_gain, alia_focus_notification)           \
    X(0x51, INPUT, FOCUS_LOSS, focus_loss, alia_focus_notification)           \
    X(0x52, INPUT, FOCUS_PREDECESSOR, focus_predecessor, alia_focus_query)    \
    X(0x53, INPUT, FOCUS_SUCCESSOR, focus_successor, alia_focus_query)        \
    X(0x54, INPUT, FOCUS_RECOVERY, focus_recovery, alia_focus_recovery)       \
    /* mouse */                                                               \
    X(0x60, INPUT, MOUSE_PRESS, mouse_press, alia_mouse_button)               \
    X(0x61, INPUT, DOUBLE_CLICK, double_click, alia_mouse_button)             \
    X(0x62, INPUT, MOUSE_RELEASE, mouse_release, alia_mouse_button)           \
    X(0x63, INPUT, MOUSE_MOTION, mouse_motion, alia_mouse_motion)             \
    X(0x64, INPUT, MOUSE_WHEEL, mouse_wheel, alia_mouse_wheel)                \
    X(0x65, INPUT, MOUSE_GAIN, mouse_gain, alia_mouse_notification)           \
    X(0x66, INPUT, MOUSE_LOSS, mouse_loss, alia_mouse_notification)           \
    X(0x67, INPUT, MOUSE_HOVER, mouse_hover, alia_mouse_notification)
/* misc */
// X(0x80, NONE, WRAPPED, wrapped, wrapped_event_payload)
// X(0x81, NONE, SET_VALUE, set_value, set_value_payload)
// X(0x82, INPUT, TIMER, timer, timer_payload)
// X(0x83, NONE, RESOLVE_LOCATION, resolve_location, resolve_location_payload)
// X(0x84, NONE, SHUTDOWN, shutdown, ui_no_payload)
// X(0x85, NONE, CUSTOM, custom, custom_event_payload)

// EVENT CODES

#define ALIA_DEFINE_EVENT_CODE(code, CATEGORY, NAME, name, data_type)         \
    ALIA_EVENT_##NAME = (code),

enum
{
    ALIA_ENUMERATE_EVENT_TYPES(ALIA_DEFINE_EVENT_CODE)
};

#undef ALIA_DEFINE_EVENT_CODE

// EVENT DATA STRUCTS

typedef struct
{
    int dummy;
} alia_nil;

typedef struct
{
    int dummy;
} alia_refresh;
typedef struct
{
    int dummy;
} alia_render;

typedef struct
{
} alia_make_widget_visible;
typedef struct
{
} alia_mouse_hit_test;
typedef struct
{
    int dummy;
} alia_wheel_hit_test;
typedef struct
{
    int dummy;
} alia_mouse_cursor_query;

typedef struct
{
    int dummy;
} alia_text_input;
typedef struct
{
    int key_code;
    int modifiers;
} alia_key_input;

typedef struct
{
    int dummy;
} alia_focus_notification;
typedef struct
{
    int dummy;
} alia_focus_query;
typedef struct
{
    int dummy;
} alia_focus_recovery;

typedef struct
{
    int button;
    float x;
    float y;
} alia_mouse_button;
typedef struct
{
    float x;
    float y;
    float last_x;
    float last_y;
} alia_mouse_motion;
typedef struct
{
    float delta;
} alia_mouse_wheel;
typedef struct
{
    int dummy;
} alia_mouse_notification;

typedef struct
{
    int dummy;
} alia_wrapped_event;
typedef struct
{
    int dummy;
} alia_set_value;
typedef struct
{
    int timer_id;
    int milliseconds;
} alia_timer;
typedef struct
{
    int dummy;
} alia_resolve_location;
typedef struct
{
    int dummy;
} alia_custom_event;

// EVENT STRUCT

#define ALIA_EVENT_PAYLOAD(code, CATEGORY, NAME, name, data_type)             \
    data_type name;

typedef struct alia_event
{
    alia_category_id category; // ALIA_CATEGORY_*
    alia_event_code type; // ALIA_EVENT_*
    union
    {
        ALIA_ENUMERATE_EVENT_TYPES(ALIA_EVENT_PAYLOAD)
    };
} alia_event;

#undef ALIA_EVENT_PAYLOAD

// CONSTRUCTORS

#define ALIA_DEFINE_EVENT_CONSTRUCTOR(code, CATEGORY, NAME, name, data_type)  \
    static inline alia_event alia_make_##name##_event(data_type data)         \
    {                                                                         \
        alia_event event;                                                     \
        event.category = ALIA_CATEGORY_##CATEGORY;                            \
        event.type = ALIA_EVENT_##NAME;                                       \
        event.name = data;                                                    \
        return event;                                                         \
    }

ALIA_ENUMERATE_EVENT_TYPES(ALIA_DEFINE_EVENT_CONSTRUCTOR)

#undef ALIA_DEFINE_EVENT_CONSTRUCTOR

#ifdef __cplusplus
} // extern "C"
#endif
