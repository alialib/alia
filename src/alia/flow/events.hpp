#ifndef ALIA_FLOW_EVENTS_HPP
#define ALIA_FLOW_EVENTS_HPP

#include <alia/components/context.hpp>
#include <alia/components/system.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/flow/macros.hpp>

// This file implements utilities for routing events through an alia content
// traversal function.
//
// In alia, the application defines the contents of the scene dynamically by
// iterating through the current objects in the scene and calling a
// library-provided function to specify the existence of each of them.
//
// This traversal function serves as a way to dispatch and handle events.
// However, in cases where the event only needs to go to a single object in the
// scene, the overhead of having the traversal function visit every other object
// can be problematic. The overhead can be reduced by having the traversal
// function skip over subregions of the scene when they're not relevant to the
// event.
//
// This file provides a system for defining a hierarchy of such subregions in
// the scene, identifying which subregion an event is targeted at, and culling
// out irrelevant subregions.

namespace alia {

struct system;
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
    routing_region_ptr* active_region = 0;
    bool targeted;
    event_routing_path* path_to_target = 0;
    std::type_info const* event_type;
    void* event;
};

template<class Context>
routing_region_ptr
get_active_routing_region(Context ctx)
{
    event_traversal& traversal = get_event_traversal(ctx);
    return traversal.active_region ? *traversal.active_region
                                   : routing_region_ptr();
}

namespace impl {

// Set up the event traversal so that it will route the control flow to the
// given target. (And also invoke the traversal.)
// :target can be null, in which case no (further) routing will be done.
void
route_event(system& sys, event_traversal& traversal, routing_region* target);

template<class Event>
void
dispatch_targeted_event(
    system& sys, Event& event, routing_region_ptr const& target)
{
    event_traversal traversal;
    traversal.targeted = true;
    traversal.event_type = &typeid(Event);
    traversal.event = &event;
    route_event(sys, traversal, target.get());
}

template<class Event>
void
dispatch_event(system& sys, Event& event)
{
    event_traversal traversal;
    traversal.targeted = false;
    traversal.event_type = &typeid(Event);
    traversal.event = &event;
    route_event(sys, traversal, 0);
}

} // namespace impl

template<class Event>
void
dispatch_event(system& sys, Event& event)
{
    impl::dispatch_event(sys, event);
    refresh_system(sys);
}

struct traversal_aborted
{
};

void
abort_traversal(dataless_context ctx);

struct scoped_routing_region
{
    scoped_routing_region() : traversal_(0)
    {
    }
    scoped_routing_region(context ctx)
    {
        begin(ctx);
    }
    ~scoped_routing_region()
    {
        end();
    }

    void
    begin(context ctx);

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

template<class Event>
bool
detect_event(dataless_context ctx, Event** event)
{
    event_traversal& traversal = get_event_traversal(ctx);
    if (*traversal.event_type == typeid(Event))
    {
        *event = reinterpret_cast<Event*>(traversal.event);
        return true;
    }
    return false;
}

template<class Event, class Context, class Handler>
void
on_event(Context ctx, Handler&& handler)
{
    Event* e;
    ALIA_UNTRACKED_IF(detect_event(ctx, &e))
    {
        handler(ctx, *e);
    }
    ALIA_END
}

struct node_identity
{
};
typedef node_identity const* node_id;

inline node_id
get_node_id(context ctx)
{
    return &get_cached_data<node_identity>(ctx);
}

// routable_node_id identifies a node with enough information that an event can
// be routed to it.
struct routable_node_id
{
    node_id id = nullptr;
    routing_region_ptr region;
};

inline routable_node_id
make_routable_node_id(node_id id, routing_region_ptr region)
{
    routable_node_id routable;
    routable.id = id;
    routable.region = region;
    return routable;
}

inline routable_node_id
make_routable_node_id(dataless_context ctx, node_id id)
{
    return make_routable_node_id(id, get_active_routing_region(ctx));
}

static routable_node_id const null_node_id;

// Is the given routable_node_id valid?
// (Only the null_node_id is invalid.)
inline bool
is_valid(routable_node_id const& id)
{
    return id.id != nullptr;
}

struct targeted_event
{
    node_id target_id;
};

template<class Event>
void
dispatch_targeted_event(system& sys, Event& event, routable_node_id const& id)
{
    event.target_id = id.id;
    impl::dispatch_targeted_event(sys, event, id.region);
    refresh_system(sys);
}

template<class Event>
bool
detect_targeted_event(dataless_context ctx, node_id id, Event** event)
{
    return detect_event(ctx, event) && (*event)->target_id == id;
}

template<class Event, class Context, class Handler>
void
on_targeted_event(Context ctx, node_id id, Handler&& handler)
{
    Event* e;
    ALIA_UNTRACKED_IF(detect_targeted_event(ctx, id, &e))
    {
        handler(ctx, *e);
        abort_traversal(ctx);
    }
    ALIA_END
}

// the refresh event...

struct refresh_event
{
};

inline bool
is_refresh_event(dataless_context ctx)
{
    refresh_event* e;
    return detect_event(ctx, &e);
}

template<class Context, class Handler>
void
on_refresh(Context ctx, Handler handler)
{
    ALIA_UNTRACKED_IF(is_refresh_event(ctx))
    {
        handler(ctx);
    }
    ALIA_END
}

} // namespace alia

#endif
