#ifndef ALIA_CONTEXT_HPP
#define ALIA_CONTEXT_HPP

#include <alia/data.hpp>
#include <alia/box.hpp>
#include <alia/event.hpp>
#include <alia/font.hpp>
#include <alia/typedefs.hpp>
#include <alia/color.hpp>
#include <alia/matrix.hpp>
#include <list>
#include <map>
#include <cassert>

namespace alia {

struct pass_state
{
    // for data tree traversal
    naming_map* active_map;
    data_block* active_block;
    dynamic_block_node* predicted_dynamic_block;
    data_node** next_data_ptr;
    // style-related state
    unsigned style_code;
    vector2i padding_size;
    font active_font;
    rgba8 text_color, bg_color, selected_text_color, selected_bg_color;
    // geometric state
    box2i clip_region; // in surface coordinates
    box2d untransformed_clip_region; // in current frame of reference
    box2i integer_untransformed_clip_region;
    matrix<3,3,double> transformation, inverse_transformation;
    // the mouse position (un)transformed into the current frame of reference
    point2d mouse_position;
    // the same, but converted to integers (via floor())
    point2i integer_mouse_position;
};

struct context
{
    context() : refresh_counter(0), pass_counter(0), map_list(0),
        mouse_button_state(0), mouse_in_surface(0), hot_id(0), active_id(0),
        focused_id(0), font_size_adjustment(0) {}
    uint64 refresh_counter;
    // associated objects
    alia::surface* surface;
    alia::artist* artist;
    alia::controller* controller;
    // for data management
    unsigned pass_counter;
    data_block root_block;
    naming_map* map_list;
    // input state
    unsigned mouse_button_state;
    bool mouse_in_surface;
    point2i mouse_position;
    region_id hot_id, active_id, focused_id;
    // current size of UI contents
    vector2i content_size;
    // font info
    std::map<font,font_metrics> cached_font_metrics;
    float font_size_adjustment;
    // only valid while issuing an event
    alia::event* event;
    alia::pass_state pass_state;
};

template<class Event>
Event& get_event(context& ctx)
{
    if (!dynamic_cast<Event*>(ctx.event))
        assert(dynamic_cast<Event*>(ctx.event));
    return static_cast<Event&>(*ctx.event);
}

template<class T>
bool get_data(context& ctx, T** ptr)
{
    data_node**& next_data_ptr = ctx.pass_state.next_data_ptr;
    data_node* node = *next_data_ptr;
    if (node)
    {
        typed_data_node<T>* typed_node =
            static_cast<typed_data_node<T>*>(node);
        assert(dynamic_cast<typed_data_node<T>*>(node));
        next_data_ptr = &node->next;
        *ptr = &typed_node->value;
        return false;
    }
    typed_data_node<T>* new_node = new typed_data_node<T>;
    *next_data_ptr = new_node;
    next_data_ptr = &new_node->next;
    *ptr = &new_node->value;
    return true;
}

template<class T>
T* get_data(context& ctx)
{
    T* ptr;
    get_data(ctx, &ptr);
    return ptr;
}

void issue_event(context& ctx, event& event);

void set_clip_region(context& ctx, box2i const& region);
void set_transformation(context& ctx,
    matrix<3,3,double> const& transformation);

void end_pass(context& ctx);

}

#endif
