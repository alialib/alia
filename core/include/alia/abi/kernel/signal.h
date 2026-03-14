#ifndef ALIA_ABI_KERNEL_SIGNAL_H
#define ALIA_ABI_KERNEL_SIGNAL_H

#include <alia/abi/prelude.h>

#include <stdbool.h>
#include <stdint.h>

ALIA_EXTERN_C_BEGIN

typedef uint32_t alia_signal_flags;

#define ALIA_SIGNAL_READABLE (1u << 0)
#define ALIA_SIGNAL_WRITABLE (1u << 1)
#define ALIA_SIGNAL_WRITTEN  (1u << 2)

typedef struct alia_bool_signal
{
    alia_signal_flags flags;
    bool value;
} alia_bool_signal;

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_SIGNAL_H */

