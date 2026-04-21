#ifndef ALIA_ABI_BASE_ALLOCATOR_H
#define ALIA_ABI_BASE_ALLOCATOR_H

#include <alia/abi/prelude.h>

// TODO: Flesh this out.

ALIA_EXTERN_C_BEGIN

typedef struct alia_general_allocator
{
    void* (*alloc)(void* user_data, size_t size, size_t alignment);
    void (*free)(void* user_data, void* ptr, size_t size, size_t alignment);
    void* user_data;
} alia_general_allocator;

ALIA_EXTERN_C_END

#endif // ALIA_ABI_BASE_ALLOCATOR_H
