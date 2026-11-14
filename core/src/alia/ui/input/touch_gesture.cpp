#include <alia/abi/ui/input/touch_gesture.h>

#include <alia/abi/ui/regions.h>
#include <alia/ui/system/object.h>
#include <alia/ui/system/work_internal.h>

using namespace alia;

extern "C" {

alia_ui_touch_gesture_resolution
alia_ui_resolve_touch_gesture(alia_ui_system* ui, alia_vec2f position)
{
    ALIA_ASSERT(ui);

    run_layout_resolve(*ui);
    rebuild_regions(*ui);

    ui->input.mouse_position = position;
    ui->input.mouse_inside_window = true;

    alia_touch_gesture_hit_test_result hit{};
    alia_ui_touch_gesture_hit_test(ui, position.x, position.y, &hit);

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
