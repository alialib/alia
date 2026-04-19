#ifndef ALIA_ABI_ROUTING_H
#define ALIA_ABI_ROUTING_H

#include <alia/abi/context.h>
#include <alia/abi/prelude.h>

#include <alia/abi/kernel/substrate.h>

ALIA_EXTERN_C_BEGIN

typedef uint32_t alia_route_node_id;

typedef struct alia_element_id
{
    void* ptr;
    alia_generation_counter generation;
    alia_route_node_id route;
} alia_element_id;

#define ALIA_ELEMENT_ID_NONE ((alia_element_id) NULL)

static inline alia_element_id
alia_offset_id(alia_element_id main_id, uint8_t index)
{
    return ALIA_BRACED_INIT(
        alia_element_id, (uint8_t*) main_id.ptr + index, main_id.generation);
}

static inline bool
alia_element_id_is_valid(alia_element_id id)
{
    return id.ptr != NULL;
}

static inline bool
alia_element_id_equal(alia_element_id a, alia_element_id b)
{
    return a.ptr == b.ptr && a.generation == b.generation
        && a.route == b.route;
}

ALIA_DEFINE_EQUALITY_OPERATOR(alia_element_id);

static inline alia_element_id
alia_make_element_id(alia_context* ctx, alia_substrate_usage_result result)
{
    // TODO: Get the route node ID from the context.
    return ALIA_BRACED_INIT(alia_element_id, result.ptr, result.generation, 0);
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_ROUTING_H
