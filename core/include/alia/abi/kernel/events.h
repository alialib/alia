#ifndef ALIA_ABI_KERNEL_EVENTS_H
#define ALIA_ABI_KERNEL_EVENTS_H

#include <alia/abi/ids.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef uint16_t alia_event_category;
typedef uint16_t alia_event_type;

// TODO: Allow dynamic payload sizes.
#define ALIA_EVENT_PAYLOAD_SIZE_MAX 64u

typedef struct alia_event
{
    alia_event_category category;
    alia_event_type type;
    alia_element_id target;
    // TODO: flags
    uint8_t payload[ALIA_EVENT_PAYLOAD_SIZE_MAX];
    uint32_t payload_size;
} alia_event;

// kernel-level categories
enum
{
    ALIA_CATEGORY_NONE = 0,
    ALIA_CATEGORY_REFRESH = 1,
};

// kernel-level types
enum
{
    ALIA_TYPE_NONE = 0,
    ALIA_TYPE_REFRESH = 1,
};

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_EVENTS_H */
