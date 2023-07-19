#ifndef ALIA_INDIE_RENDERING_HPP
#define ALIA_INDIE_RENDERING_HPP

#include <alia/indie/common.hpp>

#include <include/core/SkCanvas.h>

#include <alia/indie/events.hpp>

namespace alia { namespace indie {

struct widget
{
    virtual void
    render(SkCanvas& canvas)
        = 0;

    virtual void
    process_region_event(region_event& event);

    widget* next = nullptr;
};

struct widget_container : widget
{
    widget* children = nullptr;
};

void
render_children(SkCanvas& canvas, widget_container& container);

struct render_traversal
{
    widget** next_ptr = nullptr;
};

void
add_widget(render_traversal& traversal, widget* node);

struct scoped_widget_container
{
    ~scoped_widget_container()
    {
        end();
    }

    void
    begin(render_traversal& traversal, widget_container* container);

    void
    end();

 private:
    render_traversal* traversal_ = nullptr;
    widget_container* container_;
};

}} // namespace alia::indie

#endif
