#ifndef ALIA_ABI_KERNEL_OBJECT_H
#define ALIA_ABI_KERNEL_OBJECT_H

#include <alia/abi/base/memory.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_kernel alia_kernel;

// size and alignment of `alia_kernel`
alia_struct_spec
alia_kernel_object_spec(void);

// Initialize a kernel in `object_storage`.
// `object_storage` must point to at least `alia_kernel_object_spec().size`
// bytes aligned to `alia_kernel_object_spec().align`.
// Pools are registered but not initialized until the first pass.
alia_kernel*
alia_kernel_init(void* object_storage, alia_memory_ops memory_ops);

// Release resources owned by `kernel` and end its lifetime.
void
alia_kernel_destroy(alia_kernel* kernel);

// Mark `kernel` as needing a refresh pass.
void
alia_kernel_mark_dirty(alia_kernel* kernel);

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_KERNEL_OBJECT_H */
