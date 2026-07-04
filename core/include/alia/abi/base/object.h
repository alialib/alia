#ifndef ALIA_ABI_BASE_OBJECT_H
#define ALIA_ABI_BASE_OBJECT_H

#include <alia/abi/prelude.h>

#include <stdlib.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

ALIA_EXTERN_C_BEGIN

// Allocate storage suitable for placement initialization of an opaque object
// described by `spec`. Returns null on failure.
static inline void*
alia_object_alloc(alia_struct_spec spec)
{
    if (spec.size == 0 || spec.align == 0)
        return NULL;

#if defined(_MSC_VER)
    return _aligned_malloc(spec.size, spec.align);
#else
    size_t const size = alia_align_up(spec.size, spec.align);
    return aligned_alloc(spec.align, size);
#endif
}

static inline void
alia_object_free(void* storage)
{
    if (!storage)
        return;

#if defined(_MSC_VER)
    _aligned_free(storage);
#else
    free(storage);
#endif
}

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_BASE_OBJECT_H */
