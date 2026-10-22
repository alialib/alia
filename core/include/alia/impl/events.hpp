#pragma once

#include <alia/abi/ui/context.h>
#include <alia/abi/ui/events.h>
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
};

} // extern "C"

namespace alia {

using event_traversal = alia_event_traversal;

template<class Context>
component_container_ptr const
get_active_component_container(Context ctx)
{
    return *alia_ctx_events(alia_ctx_kernel(ctx))->active_container;
}

using component_identity = std::weak_ptr<component_container>;

inline alia_event_category
get_event_category(alia_ui_context& ctx)
{
    return alia_ctx_events(alia_ctx_kernel(&ctx))->event->category;
}

inline alia_event_category
get_event_category(ui_context ctx)
{
    return get_event_category(*ctx);
}

inline alia_event_type
get_event_type(alia_ui_context& ctx)
{
    return alia_ctx_events(alia_ctx_kernel(&ctx))->event->type;
}

inline alia_event_type
get_event_type(ui_context ctx)
{
    return get_event_type(*ctx);
}

inline alia_event_type
get_event_type(alia_kernel_context& ctx)
{
    return alia_ctx_events(&ctx)->event->type;
}

inline alia_event_type
get_event_type(kernel_context ctx)
{
    return get_event_type(*ctx);
}

inline alia_element_id
get_event_target(alia_ui_context& ctx)
{
    return alia_ctx_events(alia_ctx_kernel(&ctx))->event->target;
}

inline alia_element_id
get_event_target(ui_context ctx)
{
    return get_event_target(*ctx);
}

inline bool
is_refresh_event(alia_ui_context& ctx)
{
    return get_event_type(ctx) == ALIA_EVENT_REFRESH;
}

inline bool
is_refresh_event(ui_context ctx)
{
    return is_refresh_event(*ctx);
}

inline bool
is_refresh_event(alia_kernel_context& ctx)
{
    return get_event_type(ctx) == ALIA_EVENT_REFRESH;
}

inline bool
is_refresh_event(kernel_context ctx)
{
    return is_refresh_event(*ctx);
}

// ACCESSORS

template<class Payload>
inline Payload&
unsafe_get_event_payload(alia_ui_context& ctx)
{
    return *reinterpret_cast<Payload*>(
        alia_ctx_events(alia_ctx_kernel(&ctx))->event->payload);
}

template<class Payload>
inline Payload&
unsafe_get_event_payload(ui_context ctx)
{
    return unsafe_get_event_payload<Payload>(*ctx);
}

#define X(code, CATEGORY, flags, NAME, name, data_type)                       \
    inline data_type& as_##name##_event(alia_ui_context& ctx)                 \
    {                                                                         \
        ALIA_ASSERT(get_event_type(ctx) == ALIA_EVENT_##NAME);                \
        return *reinterpret_cast<data_type*>(                                 \
            alia_ctx_events(alia_ctx_kernel(&ctx))->event->payload);          \
    }                                                                         \
    inline data_type& as_##name##_event(ui_context ctx)                       \
    {                                                                         \
        return as_##name##_event(*ctx);                                       \
    }                                                                         \
    inline data_type& as_##name##_event(alia_kernel_context& ctx)             \
    {                                                                         \
        ALIA_ASSERT(get_event_type(ctx) == ALIA_EVENT_##NAME);                \
        return *reinterpret_cast<data_type*>(                                 \
            alia_ctx_events(&ctx)->event->payload);                           \
    }                                                                         \
    inline data_type& as_##name##_event(kernel_context ctx)                   \
    {                                                                         \
        return as_##name##_event(*ctx);                                       \
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
// detect_event(ui_context& ctx, Event** event)
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
refresh_component_identity(ui_context ctx, component_identity& identity);

component_id
get_component_id(ui_context ctx);

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
detect_targeted_event(ui_context ctx, component_id id, Event** event)
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
