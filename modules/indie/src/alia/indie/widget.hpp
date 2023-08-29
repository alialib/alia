#ifndef ALIA_INDIE_RENDERING_HPP
#define ALIA_INDIE_RENDERING_HPP

#include <memory>

#include <include/core/SkCanvas.h>

#include <alia/core/flow/events.hpp>
#include <alia/indie/common.hpp>
#include <alia/indie/context.hpp>
#include <alia/indie/events/input.hpp>
#include <alia/indie/layout/utilities.hpp>

namespace alia { namespace indie {

struct hit_test_base;

struct widget : std::enable_shared_from_this<widget>
{
    virtual void
    render(SkCanvas& canvas)
        = 0;

    virtual void
    hit_test(hit_test_base& test) const
        = 0;

    virtual void
    process_input(event_context ctx)
        = 0;

    // virtual component_identity
    // identity() const
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
    // TODO: Get rid of the const_cast.
    return external_widget_handle(
        const_cast<indie::widget*>(widget)->shared_from_this());
}

}} // namespace alia::indie

#endif
