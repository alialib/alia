#ifndef ALIA_HOST_FRAME_H
#define ALIA_HOST_FRAME_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_ui_system alia_ui_system;

// Invoked by a platform host to run one application frame (update/draw).
typedef void (*alia_host_frame_fn)(void* user_data);

typedef struct alia_host_frame_handler
{
    alia_host_frame_fn fn;
    void* user_data;
} alia_host_frame_handler;

ALIA_EXTERN_C_END

#endif /* ALIA_HOST_FRAME_H */
