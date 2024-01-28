#ifndef ALIA_UI_RENDERING_HPP
#define ALIA_UI_RENDERING_HPP

#include "alia/ui/layout/specification.hpp"
#include <memory>

#include <include/core/SkCanvas.h>

#include <alia/core/flow/events.hpp>
#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/geometry.hpp>

namespace alia {

struct hit_test_base;

struct region_reveal_request
{
    layout_box region;
    // If this is set, the UI will jump abruptly instead of smoothly scrolling.
    bool abrupt;
    // If this is set, the region will be moved to the top of the UI instead
    // of just being made visible.
    bool move_to_top;
};

struct render_event
{
    ui_system* sys = nullptr;

    SkCanvas* canvas = nullptr;

    layout_vector current_offset = make_layout_vector(0, 0);
};

struct widget_interface
{
    virtual void
    render(render_event& event)
        = 0;

    virtual void
    hit_test(hit_test_base& test, vector<2, double> const& point) const
        = 0;

    virtual void
    process_input(ui_event_context ctx)
        = 0;

    // Get the transformation from the root frame of reference to this widget's
    // frame of reference.
    // Note that there are no particular geometric guarantees about a widget's
    // frame of refernece. (e.g., The origin of this frame is often the
    // top-left corner of the widget's bounding box, but this isn't
    // guaranteed.)
    virtual matrix<3, 3, double>
    transformation() const = 0;

    // Get the bounding box for this widget in its own frame of reference.
    // The bounding box can
    virtual layout_box
    bounding_box() const
        = 0;

    virtual void
    reveal_region(region_reveal_request const& request)
        = 0;
};

struct widget : layout_node_interface,
                widget_interface,
                std::enable_shared_from_this<widget>
{
    ~widget()
    {
    }

    // the next widget in the widget's sibling list
    widget* next = nullptr;

    // the first child of this widget
    widget* children = nullptr;

    // the parent of the widget
    widget_container* parent = nullptr;
};

struct widget_container : widget
{
    // This records the last refresh in which the contents of the container
    // changed. It's updated during the refresh pass and is used to determine
    // when the container's layout needs to be recomputed.
    counter_type last_content_change = 1;

    virtual void
    record_content_change(
        layout_traversal<widget_container, widget>& traversal);
};

void
render_children(render_event& event, widget_container& container);

struct internal_widget_ref
{
    internal_widget_ref()
    {
    }

    internal_widget_ref(widget const& widget)
        : raw_ptr(const_cast<alia::widget*>(&widget))
    {
    }

    explicit operator bool() const
    {
        return raw_ptr != nullptr;
    }

    widget const&
    operator*() const
    {
        return *raw_ptr;
    }

    widget const*
    operator->() const
    {
        return raw_ptr;
    }

    widget* raw_ptr = nullptr;
};

inline bool
operator==(internal_widget_ref const& a, internal_widget_ref const& b)
{
    return a.raw_ptr == b.raw_ptr;
}
inline bool
operator!=(internal_widget_ref const& a, internal_widget_ref const& b)
{
    return !(a == b);
}

struct external_widget_ref
{
    external_widget_ref()
    {
    }

    external_widget_ref(std::shared_ptr<widget> widget)
        : internal_(*widget), ownership_(widget)
    {
    }

    explicit operator bool() const noexcept
    {
        return internal_ != internal_widget_ref{};
    }

    bool
    matches(internal_widget_ref internal) const noexcept
    {
        return internal_ == internal;
    }

    std::shared_ptr<widget>
    lock() const noexcept
    {
        return ownership_.lock();
    }

    internal_widget_ref
    raw_ptr() const
    {
        return internal_;
    }

 private:
    // maintained for comparison purposes
    internal_widget_ref internal_;
    // maintained for externally invoking
    std::weak_ptr<widget> ownership_;
};

inline bool
operator==(external_widget_ref const& a, external_widget_ref const& b)
{
    return a.raw_ptr() == b.raw_ptr();
}
inline bool
operator!=(external_widget_ref const& a, external_widget_ref const& b)
{
    return !(a == b);
}

inline external_widget_ref
externalize(internal_widget_ref widget)
{
    if (widget)
    {
        return external_widget_ref(widget.raw_ptr->shared_from_this());
    }
    else
    {
        return external_widget_ref();
    }
}

struct internal_element_ref
{
    internal_widget_ref widget;
    int id;

    explicit operator bool() const
    {
        return widget ? true : false;
    }
};

inline bool
operator==(internal_element_ref const& a, internal_element_ref const& b)
{
    return a.widget == b.widget && a.id == b.id;
}
inline bool
operator!=(internal_element_ref const& a, internal_element_ref const& b)
{
    return !(a == b);
}

struct external_element_ref
{
    external_widget_ref widget;
    int id;

    explicit operator bool() const
    {
        return widget ? true : false;
    }

    bool
    matches(internal_element_ref internal) const noexcept
    {
        return this->widget.matches(internal.widget)
               && this->id == internal.id;
    }
};

inline bool
operator==(external_element_ref const& a, external_element_ref const& b)
{
    return a.widget == b.widget && a.id == b.id;
}
inline bool
operator!=(external_element_ref const& a, external_element_ref const& b)
{
    return !(a == b);
}

inline external_element_ref
externalize(internal_element_ref element)
{
    return external_element_ref{externalize(element.widget), element.id};
}

template<class Visitor>
void
walk_widget_tree(widget* widget, Visitor&& visitor)
{
    // Visit this node.
    std::forward<Visitor>(visitor)(widget);
    // Visit all its children.
    for (auto* w = widget->children; w != nullptr; w = w->next)
    {
        walk_widget_tree(w, std::forward<Visitor>(visitor));
    }
}

} // namespace alia

#endif
