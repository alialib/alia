#ifndef ALIA_ABI_UI_SYSTEM_WORK_H
#define ALIA_ABI_UI_SYSTEM_WORK_H

#include <alia/abi/kernel/events.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

void
alia_ui_enqueue_event(alia_ui_system* ui, alia_event const* event);

void
alia_ui_mark_dirty(alia_ui_system* ui);

typedef enum alia_ui_refresh_hook_policy
{
    ALIA_UI_REFRESH_NEVER = 0,
    ALIA_UI_REFRESH_IF_DIRTY = 1,
    ALIA_UI_REFRESH_ALWAYS = 2,
} alia_ui_refresh_hook_policy;

typedef struct alia_ui_refresh_policy
{
    alia_ui_refresh_hook_policy before_input;
    alia_ui_refresh_hook_policy before_draw;
} alia_ui_refresh_policy;

void
alia_ui_set_refresh_policy(
    alia_ui_system* ui, alia_ui_refresh_policy const* policy);

typedef enum alia_ui_work_kind
{
    ALIA_UI_WORK_LAYOUT_RESOLVE,
    ALIA_UI_WORK_EVENT_DISPATCH,
} alia_ui_work_kind;

typedef struct alia_ui_work_item
{
    alia_ui_work_kind kind;
    alia_event event; // for ALIA_UI_WORK_EVENT_DISPATCH
} alia_ui_work_item;

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_UI_SYSTEM_WORK_H */
