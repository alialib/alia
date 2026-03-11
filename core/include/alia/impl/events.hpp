#pragma once

#include <alia/abi/context.h>
#include <alia/abi/events.h>
#include <alia/kernel/flow/traversal.h>

namespace alia {

struct event_routing_path
{
    component_container* node;
    event_routing_path* rest;
};

} // namespace alia

extern "C" {

struct alia_event_traversal
{
    alia::component_container_ptr* active_container = nullptr;
    bool targeted;

    alia::event_routing_path* path_to_target = nullptr;
    alia_event* event = {};
    bool aborted = false;
};

} // extern "C"

namespace alia {

template<class Context>
component_container_ptr const
get_active_component_container(Context& ctx)
{
    return *ctx.events->active_container;
}

typedef component_container_ptr::weak_type component_identity;

struct traversal_aborted
{
};

void
abort_traversal(ephemeral_context& ctx);

inline bool
traversal_was_aborted(ephemeral_context& ctx)
{
    return ctx.events->aborted;
}

inline alia_event_category
get_event_category(ephemeral_context& ctx)
{
    return ctx.events->event->category;
}

inline alia_event_type
get_event_type(ephemeral_context& ctx)
{
    return ctx.events->event->type;
}

inline alia_element_id
get_event_target(ephemeral_context& ctx)
{
    return ctx.events->event->target;
}

inline bool
is_refresh_event(ephemeral_context& ctx)
{
    return get_event_type(ctx) == ALIA_EVENT_REFRESH;
}

// ACCESSORS

#define X(code, CATEGORY, flags, NAME, name, data_type)                       \
    inline data_type& as_##name##_event(ephemeral_context& ctx)               \
    {                                                                         \
        ALIA_ASSERT(get_event_type(ctx) == ALIA_EVENT_##NAME);                \
        return *reinterpret_cast<data_type*>(ctx.events->event->payload);     \
    }

ALIA_EVENTS(X)
#undef X

#define X(code, CATEGORY, flags, NAME, name, data_type)                       \
    inline data_type const& as_##name##_event(alia_event const& event)        \
    {                                                                         \
        ALIA_ASSERT(event.type == ALIA_EVENT_##NAME);                         \
        return *reinterpret_cast<data_type const*>(event.payload);            \
    }                                                                         \
    inline data_type& as_##name##_event(alia_event& event)                    \
    {                                                                         \
        ALIA_ASSERT(event.type == ALIA_EVENT_##NAME);                         \
        return *reinterpret_cast<data_type*>(event.payload);                  \
    }

ALIA_EVENTS(X)
#undef X

// template<class Event>
// bool
// detect_event(ephemeral_context& ctx, Event** event)
// {
//     event_traversal& traversal = get_event_traversal(ctx);
//     if (traversal.event->type == ALIA_EVENT_TYPE(Event))
//     {
//         *event = reinterpret_cast<Event*>(traversal.event);
//         return true;
//     }
//     return false;
// }

// TODO: Implement this.
// template<class Event, class Context, class Handler>
// void
// event_handler(Context ctx, Handler&& handler)
// {
//     Event* e;
//     ALIA_UNTRACKED_IF(detect_event(ctx, &e))
//     {
//         handler(ctx, *e);
//     }
//     ALIA_END
// }

typedef component_identity* component_id;

void
refresh_component_identity(
    ephemeral_context& ctx, component_identity& identity);

component_id
get_component_id(ephemeral_context& ctx);

// external_component_id identifies a component in a form that can be safely
// stored outside of the alia data graph.
struct external_component_id
{
    // :id captures the original ID pointer used for the component within the
    // data graph.
    component_id id = nullptr;
    // :identity stores a copy of the actual weak_ptr to the component identity
    // so that it can be tested externally to see if the component still exists
    // within the data graph.
    component_identity identity;

    explicit
    operator bool() const
    {
        return id != nullptr;
    }
};

external_component_id const null_component_id{};

inline external_component_id
externalize(component_id id)
{
    external_component_id external;
    external.id = id;
    external.identity = *id;
    return external;
}

struct targeted_event
{
    component_id target_id;
};

template<class Event>
bool
detect_targeted_event(ephemeral_context& ctx, component_id id, Event** event)
{
    return detect_event(ctx, event) && (*event)->target_id == id;
}

// TODO: Implement this.
// template<class Event, class Context, class Handler>
// void
// targeted_event_handler(Context ctx, component_id id, Handler&& handler)
// {
//     Event* e;
//     ALIA_UNTRACKED_IF(detect_targeted_event(ctx, id, &e))
//     {
//         handler(ctx, *e);
//         abort_traversal(ctx);
//     }
//     ALIA_END
// }

// TODO: Implement this.
// template<class Context, class Handler>
// void
// refresh_handler(Context ctx, Handler handler)
// {
//     ALIA_UNTRACKED_IF(is_refresh_event(ctx))
//     {
//         handler(ctx);
//     }
//     ALIA_END
// }

} // namespace alia
