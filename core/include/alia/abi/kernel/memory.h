#ifndef ALIA_ABI_KERNEL_MEMORY_H
#define ALIA_ABI_KERNEL_MEMORY_H

#include <alia/abi/base/memory.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_kernel alia_kernel;

// kernel pool name - The value is not a pool ID.
typedef enum alia_kernel_pool_name
{
    ALIA_KERNEL_POOL_STACK = 0,
    ALIA_KERNEL_POOL_SCRATCH,
    ALIA_KERNEL_POOL_SUBSTRATE_DISCOVERY,
    ALIA_KERNEL_POOL_SUBSTRATE_STORAGE,
} alia_kernel_pool_name;

alia_memory_system*
alia_kernel_memory(alia_kernel* kernel);

// Return the pool ID registered for `name`.
alia_memory_pool_id
alia_kernel_pool(alia_kernel* kernel, alia_kernel_pool_name name);

ALIA_EXTERN_C_END

#endif
