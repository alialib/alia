#ifndef ALIA_ABI_IDS_H
#define ALIA_ABI_IDS_H

#include <alia/abi/context.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef void* alia_element_id;

#define ALIA_ELEMENT_ID_NONE ((alia_element_id) NULL)

static inline alia_element_id
alia_offset_id(alia_element_id main_id, uint8_t index)
{
    return (alia_element_id) ((uint8_t*) main_id + index);
}

typedef struct alia_route_node_id
{
    // TODO: Implement this.
    int dummy;
} alia_route_node_id;

typedef struct alia_routable_element_id
{
    alia_element_id element;
    alia_route_node_id route;
} alia_routable_element_id;

static inline bool
alia_element_id_is_valid(alia_element_id id)
{
    return id != NULL;
}

static inline bool
alia_routable_element_id_is_valid(alia_routable_element_id id)
{
    return alia_element_id_is_valid(id.element);
}

static inline bool
alia_routable_element_id_matches(
    alia_routable_element_id id, alia_element_id element)
{
    return id.element == element;
}

static inline alia_routable_element_id
alia_make_routable_element_id(alia_context* ctx, alia_element_id id)
{
    // TODO: Implement this.
    return ALIA_BRACED_INIT(alia_routable_element_id, id, {.dummy = 0});
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_IDS_H
