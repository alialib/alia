#include <alia/abi/ui/input/touch_gesture.h>

#include <alia/abi/ui/events.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/ui/system/object.h>
#include <alia/ui/system/work_internal.h>

using namespace alia;

extern "C" {

alia_ui_touch_gesture_resolution
alia_ui_resolve_touch_gesture(alia_ui_system* ui, alia_vec2f position)
{
    ALIA_ASSERT(ui);

    run_layout_resolve(*ui);

    ui->input.mouse_position = position;
    ui->input.mouse_inside_window = true;

    alia_hit_target const none = {.id = ALIA_ELEMENT_ID_NONE, .z = 0};
    alia_event hit_test_event = alia_make_touch_gesture_hit_test_event(
        {.x = position.x,
         .y = position.y,
         .result = {
             .pointer = none,
             .scroll = none,
             .touch_drag = none,
         }});
    dispatch_event(*ui, hit_test_event);

    alia_touch_gesture_hit_test_result const& hit
        = as_touch_gesture_hit_test_event(hit_test_event).result;

    if (alia_element_id_is_valid(hit.touch_drag.id))
    {
        return {
            .kind = ALIA_TOUCH_GESTURE_POINTER,
            .target = hit.touch_drag.id,
        };
    }

    if (alia_element_id_is_valid(hit.scroll.id))
    {
        return {
            .kind = ALIA_TOUCH_GESTURE_PAN_SCROLL,
            .target = hit.scroll.id,
        };
    }

    return {
        .kind = ALIA_TOUCH_GESTURE_POINTER,
        .target = hit.pointer.id,
    };
}

} // extern "C"
