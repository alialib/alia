#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* alia_element_id;

#define ALIA_ELEMENT_ID_NONE ((alia_element_id) NULL)

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

inline bool
alia_element_id_is_valid(alia_element_id id)
{
    return id != nullptr;
}

inline bool
alia_routable_element_id_is_valid(alia_routable_element_id id)
{
    return alia_element_id_is_valid(id.element);
}

#ifdef __cplusplus
}
#endif
