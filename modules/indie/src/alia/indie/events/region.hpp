#ifndef ALIA_INDIE_EVENTS_REGION_HPP
#define ALIA_INDIE_EVENTS_REGION_HPP

#include <alia/indie/events/common.hpp>
#include <alia/indie/events/defines.hpp>

namespace alia { namespace indie {

enum class region_event_type
{
    MAKE_WIDGET_VISIBLE,
    MOUSE_HIT_TEST,
    WHEEL_HIT_TEST,
};

struct region_event : ui_event
{
    region_event_type type;
    region_event(region_event_type type)
        : ui_event{event_category::REGION}, type(type)
    {
    }
};

struct hit_test_parameters
{
    vector<2, double> mouse_position;
};

struct mouse_hit_response
{
    external_component_id id;
    mouse_cursor cursor;
    layout_box hit_box;
    std::string tooltip_message;
};

struct mouse_hit_test_event
    : region_event,
      query_event<hit_test_parameters, mouse_hit_response>
{
    mouse_hit_test_event(hit_test_parameters parameters)
        : region_event(region_event_type::MOUSE_HIT_TEST),
          query_event(std::move(parameters))
    {
    }
};

// struct wheel_hit_test_event
//     : query_event<region_event, hit_test_parameters, external_component_id>
// {
//     routable_widget_id id;

//     wheel_hit_test_event()
//         : ui_event(REGION_CATEGORY, WHEEL_HIT_TEST_EVENT),
//         id(null_widget_id)
//     {
//     }
// };

// struct widget_visibility_request
// {
//     routable_component_id widget;
//     // If this is set, the UI will jump abruptly instead of smoothly
//     // scrolling;
//     bool abrupt;
//     // If this is set, the widget will be moved to the top of the UI instead
//     // of just being made visible.
//     bool move_to_top;
// };

// struct make_widget_visible_event : ui_event
// {
//     make_widget_visible_event(widget_visibility_request const& request)
//         : ui_event(REGION_CATEGORY, MAKE_WIDGET_VISIBLE_EVENT),
//           request(request),
//           acknowledged(false)
//     {
//     }
//     widget_visibility_request request;
//     // This gets filled in once we find the widget in question.
//     box<2, double> region;
//     bool acknowledged;
// };

}} // namespace alia::indie

#endif
