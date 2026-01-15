#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct alia_ui_system alia_ui_system;
typedef struct alia_layout_container alia_layout_container;
typedef struct alia_event_traversal alia_event_traversal;

typedef struct alia_style
{
    float padding;
} alia_style;

typedef struct alia_context
{
    alia_style* style;
    alia_ui_system* system;
    alia_event_traversal* event;
} alia_context;

typedef alia_context alia_ephemeral_context;

#ifdef __cplusplus
}
#endif