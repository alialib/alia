#ifndef ALIA_LAYOUT_SYSTEM_H
#define ALIA_LAYOUT_SYSTEM_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_layout_system alia_layout_system;

void
alia_layout_system_init(alia_layout_system* system);

void
alia_layout_system_resolve(
    alia_layout_system* system, alia_vec2f available_space);

ALIA_EXTERN_C_END

#endif // ALIA_LAYOUT_SYSTEM_H
