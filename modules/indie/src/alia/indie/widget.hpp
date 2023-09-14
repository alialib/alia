#ifndef ALIA_INDIE_RENDERING_HPP
#define ALIA_INDIE_RENDERING_HPP

#include <memory>

#include <include/core/SkCanvas.h>

#include <alia/core/flow/events.hpp>
#include <alia/indie/common.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/geometry.hpp>
#include <alia/indie/layout/utilities.hpp>

namespace alia { namespace indie {

struct hit_test_base;

struct widget : std::enable_shared_from_this<widget>
{
    ~widget()
    {
    }

    virtual void
    render(SkCanvas& canvas)
        = 0;

    virtual void
    hit_test(hit_test_base& test, vector<2, double> const& point) const
        = 0;

    virtual void
    process_input(event_context ctx)
        = 0;

    // layout interface
    virtual layout_requirements
    get_horizontal_requirements(layout_calculation_context& ctx)
        = 0;
    virtual layout_requirements
    get_vertical_requirements(
        layout_calculation_context& ctx, layout_scalar assigned_width)
        = 0;
    virtual void
    set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment)
        = 0;

    // next node in the list of siblings
    widget* next = nullptr;
    // children
    widget* children = nullptr;
};

struct leaf_widget : widget
{
    void
    refresh_layout(
        layout_traversal& traversal,
        layout const& layout_spec,
        leaf_layout_requirements const& requirements,
        layout_flag_set default_flags = TOP | LEFT | PADDED);

    relative_layout_assignment const&
    assignment() const
    {
        return relative_assignment_;
    }

    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements(layout_calculation_context& ctx);
    layout_requirements
    get_vertical_requirements(
        layout_calculation_context& ctx, layout_scalar assigned_width);
    void
    set_relative_assignment(
        layout_calculation_context& ctx,
        relative_layout_assignment const& assignment);

 private:
    // the resolved spec
    resolved_layout_spec resolved_spec_;

    // requirements supplied by the widget
    leaf_layout_requirements requirements_;

    // resolved relative assignment
    alia::relative_layout_assignment relative_assignment_;
};

struct widget_container : widget
{
};

void
render_children(SkCanvas& canvas, widget_container& container);

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

struct external_widget_handle
{
    external_widget_handle()
    {
    }

    external_widget_handle(std::shared_ptr<widget> widget)
        : raw_ptr_(widget.get()), ownership_(widget)
    {
    }

    explicit operator bool() const noexcept
    {
        return raw_ptr_ != nullptr;
    }

    bool
    matches(widget const* ptr) const noexcept
    {
        return raw_ptr_ == ptr;
    }

    std::shared_ptr<widget>
    lock() const noexcept
    {
        return ownership_.lock();
    }

    widget const*
    raw_ptr() const
    {
        return raw_ptr_;
    }

 private:
    // maintained for comparison purposes
    widget const* raw_ptr_ = nullptr;
    // maintained for externally invoking
    std::weak_ptr<widget> ownership_;
};

inline bool
operator==(external_widget_handle const& a, external_widget_handle const& b)
{
    return a.raw_ptr() == b.raw_ptr();
}
inline bool
operator!=(external_widget_handle const& a, external_widget_handle const& b)
{
    return !(a == b);
}

inline external_widget_handle
externalize(widget const* widget)
{
    if (widget)
    {
        // The const cast here is a bit unfortunate, but I think it's
        // reasonable to say that when you are converting an internal widget
        // reference to a form that can be handed off externally, you should
        // also expect that it might at some point be used in a non-const
        // manner.
        return external_widget_handle(
            const_cast<indie::widget*>(widget)->shared_from_this());
    }
    else
    {
        return external_widget_handle();
    }
}

template<class Visitor>
void
walk_widget_tree(widget* widget, Visitor&& visitor)
{
    // Visit this node.
    std::forward<Visitor>(visitor)(widget);
    // Visit all its children.
    for (indie::widget* w = widget->children; w != nullptr; w = w->next)
    {
        walk_widget_tree(w, std::forward<Visitor>(visitor));
    }
}

}} // namespace alia::indie

#endif
