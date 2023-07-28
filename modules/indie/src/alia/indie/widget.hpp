#ifndef ALIA_INDIE_RENDERING_HPP
#define ALIA_INDIE_RENDERING_HPP

#include <include/core/SkCanvas.h>

#include <alia/core/flow/events.hpp>
#include <alia/indie/common.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/utilities.hpp>
#include <alia/indie/utilities/hit_testing.hpp>

namespace alia { namespace indie {

struct widget
{
    virtual void
    render(SkCanvas& canvas)
        = 0;

    virtual void
    hit_test(hit_test_base& test)
        = 0;

    // virtual void
    // handle_input(input_event& event)
    //     = 0;

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
