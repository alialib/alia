#ifndef ALIA_INDIE_RENDERING_HPP
#define ALIA_INDIE_RENDERING_HPP

#include "alia/core/flow/events.hpp"
#include <alia/indie/common.hpp>

#include <include/core/SkCanvas.h>

#include <alia/indie/events/mouse.hpp>
#include <alia/indie/layout/utilities.hpp>

namespace alia { namespace indie {

enum class hit_test_type
{
    // hit testing for potential mouse interactions
    MOUSE,
    // hit testing for potential scroll wheel movement
    WHEEL
};

struct hit_test_base
{
    // the type of test
    hit_test_type type;
    // the point we're testing
    vector<2, double> point;
};

struct mouse_hit_test_result
{
    external_component_id id;
    mouse_cursor cursor;
    layout_box hit_box;
    std::string tooltip_message;
};

struct mouse_hit_test : hit_test_base
{
    std::optional<mouse_hit_test_result> result;

    mouse_hit_test(vector<2, double> point)
        : hit_test_base{hit_test_type::MOUSE, point}
    {
    }
};

struct wheel_hit_test : hit_test_base
{
    std::optional<external_component_id> result;

    wheel_hit_test(vector<2, double> point)
        : hit_test_base{hit_test_type::WHEEL, point}
    {
    }
};

struct widget
{
    virtual void
    render(SkCanvas& canvas)
        = 0;

    virtual void
    hit_test(hit_test_base& test)
        = 0;

    widget* next = nullptr;
};

struct leaf_widget : widget, layout_leaf
{
};

struct widget_container : widget
{
    widget* children = nullptr;
};

void
render_children(SkCanvas& canvas, widget_container& container);

struct widget_traversal
{
    widget** next_ptr = nullptr;
};

void
add_widget(widget_traversal& traversal, widget* node);

struct scoped_widget_container
{
    ~scoped_widget_container()
    {
        end();
    }

    void
    begin(widget_traversal& traversal, widget_container* container);

    void
    end();

 private:
    widget_traversal* traversal_ = nullptr;
    widget_container* container_;
};

}} // namespace alia::indie

#endif
