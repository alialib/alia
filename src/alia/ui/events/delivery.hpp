#ifndef ALIA_UI_EVENTS_DELIVERY_HPP
#define ALIA_UI_EVENTS_DELIVERY_HPP

#include "alia/core/flow/events.hpp"
#include <alia/ui/context.hpp>
#include <alia/ui/widget.hpp>

namespace alia {

template<class Event>
struct event_delivery_fixture
{
    event_delivery_fixture(ui_system& sys)
        : ctx_(detail::add_context_object<ui_system_tag>(
            detail::add_context_object<timing_tag>(
                detail::add_context_object<event_traversal_tag>(
                    detail::add_context_object<core_system_tag>(
                        make_context(detail::make_empty_structural_collection(
                            &storage_)),
                        std::ref(sys)),
                    std::ref(traversal_)),
                std::ref(timing_)),
            std::ref(sys)))
    {
        traversal_.is_refresh = false;
        traversal_.targeted = true;
        traversal_.event_type = &typeid(Event);

        timing_.tick_counter = sys.external->get_tick_count();

        storage_.content_id = &unit_id;
    }

    void
    deliver(widget* widget, Event& event)
    {
        traversal_.event = &event;
        try
        {
            widget->process_input(ctx_);
        }
        catch (traversal_aborted&)
        {
        }
    }

 private:
    event_traversal traversal_;
    timing_subsystem timing_;
    ui_context_storage storage_;
    ui_event_context ctx_;
};

template<class Event>
void
deliver_input_event(ui_system& sys, widget* widget, Event& event)
{
    event_delivery_fixture<Event> fixture(sys);
    fixture.deliver(widget, event);
}

template<class Event>
void
deliver_input_event(
    ui_system& sys, std::shared_ptr<widget> const& widget, Event& event)
{
    if (widget)
        deliver_input_event(sys, widget.get(), event);
}

template<class Event>
void
deliver_input_event(ui_system& sys, external_widget_ref widget, Event& event)
{
    deliver_input_event(sys, widget.lock(), event);
}

} // namespace alia

#endif
