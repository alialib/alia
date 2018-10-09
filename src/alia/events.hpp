#ifndef ALIA_EVENTS_HPP
#define ALIA_EVENTS_HPP

#include <alia/context.hpp>
#include <alia/data_graph.hpp>

// This file implements utilities for routing events through an alia content
// traversal function.
//
// In alia, the application defines the contents of the scene dynamically by
// iterating through the current objects in the scene and calling a
// library-provided function to specify the existence of each of them.
//
// This function also serves as a way to dispatch and handle events. However, in
// cases where the event only needs to go to one object in the scene, the
// overhead of having the traversal function visit every other object can be
// problematic. The overhead can be reduced by having the traversal function
// skip over subregions of the scene when they're not relevant to the event.
//
// This file provides a system for defining a hierarchy of such subregions in
// the scene, identifying which subregion an event is targeted at, and culling
// out irrelevant subregions.

namespace alia {

struct routing_region;

typedef std::shared_ptr<routing_region> routing_region_ptr;

struct routing_region
{
    routing_region_ptr parent;
};

struct event_routing_path
{
    routing_region* node;
    event_routing_path* rest;
};

struct event_traversal
{
    routing_region_ptr* active_region;
    bool targeted;
    event_routing_path* path_to_target;
    std::type_info const* event_type;
    void* event;
};

inline routing_region_ptr
get_active_routing_region(context& ctx)
{
    event_traversal& traversal = get_event_traversal(ctx);
    return traversal.active_region ? *traversal.active_region
                                   : routing_region_ptr();
}

template<class TraversalFunction>
void
invoke_targeted_traversal(
    TraversalFunction& fn, event_traversal& traversal, routing_region* target)
{
    // In order to construct the path to the target, we start at the target and
    // follow the 'parent' pointers until we reach the root.
    // We do this via recursion so that the path can be constructed entirely
    // on the stack.
    if (target)
    {
        event_routing_path path_node;
        path_node.rest = traversal.path_to_target;
        path_node.node = target;
        traversal.path_to_target = &path_node;
        invoke_targeted_traversal(fn, traversal, target->parent.get());
    }
    else
        fn();
}

template<class TraversalFunction, class Event>
void
dispatch_event(
    TraversalFunction& fn,
    event_traversal& traversal,
    bool targeted,
    Event& event,
    routing_region_ptr const& target = routing_region_ptr())
{
    traversal.active_region = 0;
    traversal.targeted = targeted;
    traversal.path_to_target = 0;
    traversal.event_type = &typeid(Event);
    traversal.event = &event;
    if (targeted)
        invoke_targeted_traversal(fn, traversal, target.get());
    else
        fn();
}

struct scoped_routing_region
{
    scoped_routing_region() : traversal_(0)
    {
    }
    scoped_routing_region(context& ctx)
    {
        begin(ctx);
    }
    ~scoped_routing_region()
    {
        end();
    }

    void
    begin(context& ctx);

    void
    end();

    bool
    is_relevant() const
    {
        return is_relevant_;
    }

 private:
    event_traversal* traversal_;
    routing_region_ptr* parent_;
    bool is_relevant_;
};

bool
detect_event(context& ctx, std::type_info const& type);

template<class Event>
bool
detect_event(context& ctx, Event** event)
{
    event_traversal& traversal = get_event_traversal(ctx);
    if (*traversal.event_type == typeid(Event))
    {
        *event = reinterpret_cast<Event*>(traversal.event);
        return true;
    }
    return false;
}

} // namespace alia

#endif
