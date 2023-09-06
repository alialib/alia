#ifndef ALIA_INDIE_EVENTS_DELIVERY_HPP
#define ALIA_INDIE_EVENTS_DELIVERY_HPP

#include <alia/indie/context.hpp>
#include <alia/indie/widget.hpp>

namespace alia { namespace indie {

template<class Event>
struct event_delivery_fixture
{
    event_delivery_fixture(system& sys)
        : ctx_(detail::add_context_object<indie::system_tag>(
            detail::add_context_object<timing_tag>(
                detail::add_context_object<event_traversal_tag>(
                    detail::add_context_object<alia::system_tag>(
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
        widget->process_input(ctx_);
    }

 private:
    event_traversal traversal_;
    timing_subsystem timing_;
    context_storage storage_;
    event_context ctx_;
};

template<class Event>
void
deliver_input_event(system& sys, widget* widget, Event& event)
{
    event_delivery_fixture<Event> fixture(sys);
    fixture.deliver(widget, event);
}

template<class Event>
void
deliver_input_event(
    system& sys, std::shared_ptr<widget> const& widget, Event& event)
{
    if (widget)
        deliver_input_event(sys, widget.get(), event);
}

template<class Event>
void
deliver_input_event(system& sys, external_widget_handle widget, Event& event)
{
    deliver_input_event(sys, widget.lock(), event);
}

}} // namespace alia::indie

#endif
