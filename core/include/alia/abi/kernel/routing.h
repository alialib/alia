#ifndef ALIA_ABI_ROUTING_H
#define ALIA_ABI_ROUTING_H

#include <alia/abi/kernel/context.h>
#include <alia/abi/prelude.h>

#include <alia/abi/kernel/substrate.h>

ALIA_EXTERN_C_BEGIN

typedef uint32_t alia_route_node_id;

// interactive element identity - This combines a stable substrate ID with a
// route node for targeted event delivery.
typedef struct alia_element_id
{
    void* ptr;
    alia_generation_counter stamp;
    alia_route_node_id route;
} alia_element_id;

#define ALIA_ELEMENT_ID_NONE ((alia_element_id) NULL)

static inline alia_substrate_id
alia_element_substrate_id(alia_element_id id)
{
    return ALIA_BRACED_INIT(alia_substrate_id, id.ptr, id.stamp);
}

static inline alia_element_id
alia_element_from_substrate_id(
    alia_substrate_id substrate, alia_route_node_id route)
{
    return ALIA_BRACED_INIT(
        alia_element_id, substrate.ptr, substrate.stamp, route);
}

// Add a sub-index to an element ID that doesn't already have one.
static inline alia_element_id
alia_element_sub_id(alia_element_id base, uint8_t sub)
{
    return alia_element_from_substrate_id(
        alia_substrate_sub_id(alia_element_substrate_id(base), sub),
        base.route);
}

static inline uint8_t
alia_element_id_sub(alia_element_id id)
{
    return alia_substrate_id_sub(alia_element_substrate_id(id));
}

static inline bool
alia_element_id_is_valid(alia_element_id id)
{
    return id.ptr != NULL;
}

static inline bool
alia_element_id_equal(alia_element_id a, alia_element_id b)
{
    return a.ptr == b.ptr && a.stamp == b.stamp && a.route == b.route;
}

ALIA_DEFINE_EQUALITY_OPERATOR(alia_element_id);

static inline alia_element_id
alia_make_element_id(
    alia_kernel_context* ctx, alia_substrate_usage_result result)
{
    // TODO: Get the route node ID from the context.
    (void) ctx;
    return alia_element_from_substrate_id(alia_make_substrate_id(result), 0);
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_ROUTING_H
