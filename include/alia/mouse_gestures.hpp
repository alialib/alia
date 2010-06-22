#ifndef ALIA_MOUSE_GESTURES_HPP
#define ALIA_MOUSE_GESTURES_HPP

#include <alia/input_defs.hpp>
#include <alia/surface.hpp>
#include <vector>

namespace alia {

enum gesture_direction { GESTURE_UP, GESTURE_DOWN, GESTURE_LEFT,
    GESTURE_RIGHT };
typedef std::vector<gesture_direction> gesture;

//struct gesture_result : control_result<gesture const*>
//{
//    bool in_progress;
//};

bool detect_mouse_gesture(context& ctx, region_id id,
    mouse_button button, rgba8 const& color, line_style const& style);

bool detect_backward_click_sequence(context& ctx, region_id id);
bool detect_forward_click_sequence(context& ctx, region_id id);

}

#endif
