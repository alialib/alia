#ifndef ALIA_ABI_KERNEL_EVENTS_H
#define ALIA_ABI_KERNEL_EVENTS_H

#include <alia/abi/kernel/ids.h>
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
    ALIA_CATEGORY_INPUT = 2,
};

// EVENT TYPES
//
// The core event types are defined via a macro table that enumerates all the
// event types.
//
// The columns are:
// `(code, category, flags, uppercase_name, lowercase_name, data_struct_name)`
//
#define ALIA_KERNEL_EVENTS(X)                                                 \
    /* none / meta */                                                         \
    X(0x00, NONE, NONE, NONE, none, alia_nil)                                 \
    /* refresh */                                                             \
    X(0x10, REFRESH, NONE, REFRESH, refresh, alia_refresh)                    \
    /* timer */                                                               \
    X(0x20, INPUT, TARGETED, TIMER, timer, alia_timer)

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_EVENTS_H */
