#ifndef ALIA_ABI_ROUTING_H
#define ALIA_ABI_ROUTING_H

#include <alia/abi/kernel/context.h>
#include <alia/abi/prelude.h>

#include <alia/abi/kernel/substrate.h>

ALIA_EXTERN_C_BEGIN

typedef uint16_t alia_route_node_id;

// interactive element identity - This pairs an instance ID with a route node
// for targeted event delivery. Multiple interactive parts of a widget take
// separate instance IDs (see `alia_substrate_use_id` /
// `alia_substrate_use_id_range`).
typedef struct alia_element_id
{
    alia_instance_id id;
    alia_route_node_id route;
} alia_element_id;

#define ALIA_ELEMENT_ID_NONE                                                  \
    ALIA_BRACED_INIT(alia_element_id, ALIA_INSTANCE_ID_NONE, 0)

static inline alia_element_id
alia_element_id_make(alia_instance_id id, alia_route_node_id route)
{
    return ALIA_BRACED_INIT(alia_element_id, id, route);
}

static inline bool
alia_element_id_is_valid(alia_element_id id)
{
    return alia_instance_id_is_valid(id.id);
}

static inline bool
alia_element_id_equal(alia_element_id a, alia_element_id b)
{
    return alia_instance_id_equal(a.id, b.id) && a.route == b.route;
}

ALIA_DEFINE_EQUALITY_OPERATOR(alia_element_id);

// Attach the current route from `ctx` to an already-owned instance ID.
static inline alia_element_id
alia_element_id_here(alia_kernel_context* ctx, alia_instance_id id)
{
    // TODO: Get the route node ID from the context.
    (void) ctx;
    return alia_element_id_make(id, 0);
}

// Use a substrate instance ID and wrap it as an element ID for the current
// route.
static inline alia_element_id
alia_element_use_id(alia_kernel_context* ctx)
{
    return alia_element_id_here(ctx, alia_substrate_use_id(ctx));
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_ROUTING_H
