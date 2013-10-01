#ifndef ALIA_EVENT_ROUTING_HPP
#define ALIA_EVENT_ROUTING_HPP

#include <alia/data_graph.hpp>

// This file defines a library for routing events through an immediate mode
// scene traversal function.
//
// When implementing a library with an immediate mode API, the application
// typically defines the contents of the scene dynamically by iterating through
// the current objects in the scene and calling a library-provided function to
// specify the existence of each of them. A sophisticated library will actually
// use this function to dispatch events or queries to these functions.
//
// However, in cases where the event only needs to go to one object in the
// scene, the overhead of having the traversal function visit every other
// object can be problematic. The overhead can be reduced by having the
// traversal function skip over subregions of the scene when they're not
// relevant to the event.
//
// This file provides a system for defining a hierarchy of such subregions in
// the scene, identifying which subregion an event is targeted at, and culling
// out irrelevant subregions.

namespace alia {

struct routing_region;

typedef alia__shared_ptr<routing_region> routing_region_ptr;

struct routing_region
{
    routing_region_ptr parent;
};

struct event_routing_path
{
    routing_region* node;
    event_routing_path* rest;
};

struct event_routing_traversal
{
    routing_region_ptr* active_region;
    data_traversal* data;
    bool targeted;
    event_routing_path* path_to_target;
};

static inline routing_region_ptr get_active_region(
    event_routing_traversal const& traversal)
{
    return traversal.active_region ? *traversal.active_region :
        routing_region_ptr();
}

template<class TraversalFunction>
void invoke_targeted_traversal(
    TraversalFunction& fn,
    event_routing_traversal& traversal,
    routing_region* target)
{
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

template<class TraversalFunction>
void invoke_routed_traversal(
    TraversalFunction& fn,
    event_routing_traversal& traversal,
    data_traversal& data,
    bool targeted,
    routing_region_ptr const& target = routing_region_ptr())
{
    traversal.active_region = 0;
    traversal.data = &data;
    traversal.targeted = targeted;
    traversal.path_to_target = 0;
    if (targeted)
        invoke_targeted_traversal(fn, traversal, target.get());
    else
        fn();
}

struct scoped_routing_region
{
    scoped_routing_region() : traversal_(0) {}
    scoped_routing_region(event_routing_traversal& traversal)
    { begin(traversal); }
    ~scoped_routing_region()
    { end(); }
    void begin(event_routing_traversal& traversal);
    void end();
    bool is_relevant() const { return is_relevant_; }
 private:
    event_routing_traversal* traversal_;
    routing_region_ptr* parent_;
    bool is_relevant_;
};

}

#endif
