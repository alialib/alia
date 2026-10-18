#include <alia/kernel/object.h>

#include <alia/base/stack.h>
#include <alia/impl/base/arena.hpp>
#include <alia/kernel/substrate.h>

#include <cstdlib>

namespace {

void*
kernel_block_alloc(void* user, size_t size, size_t alignment)
{
    (void) user;
    (void) alignment;
    return malloc(size);
}

void
kernel_block_free(void* user, void* ptr, size_t size, size_t alignment)
{
    (void) user;
    (void) size;
    (void) alignment;
    free(ptr);
}

} // namespace

namespace alia {

void
init_kernel(alia_kernel& kernel)
{
    void* block = allocate_virtual_block(1024 * 1024);
    kernel.stack_buffer = block;
    kernel.stack_capacity = 1024 * 1024;
    alia_stack_init(&kernel.stack, block, kernel.stack_capacity);

    initialize_lazy_commit_arena(&kernel.substrate_discovery_arena);
    initialize_lazy_commit_arena(&kernel.scratch, 1024 * 1024);

    alia_general_allocator allocator;
    allocator.user_data = nullptr;
    allocator.alloc = kernel_block_alloc;
    allocator.free = kernel_block_free;
    substrate_system_init(kernel.substrate, allocator);

    kernel.dirty = true;
}

void
release_kernel(alia_kernel& kernel)
{
    substrate_system_destroy(kernel.substrate);
    alia_arena_destroy(&kernel.substrate_discovery_arena);
    alia_arena_destroy(&kernel.scratch);

    if (kernel.stack_buffer)
    {
        release_virtual_block(kernel.stack_buffer, kernel.stack_capacity);
        kernel.stack_buffer = nullptr;
        kernel.stack_capacity = 0;
        kernel.stack.base = nullptr;
        kernel.stack.capacity = 0;
        kernel.stack.top = 0;
        kernel.stack.top_entry_size = 0;
    }
}

} // namespace alia

extern "C" {

alia_struct_spec
alia_kernel_object_spec(void)
{
    return alia_struct_spec{
        .size = sizeof(alia_kernel), .align = alignof(alia_kernel)};
}

alia_kernel*
alia_kernel_init(void* object_storage)
{
    ALIA_ASSERT(object_storage);

    alia_kernel* kernel = new (object_storage) alia_kernel;
    alia::init_kernel(*kernel);
    return kernel;
}

void
alia_kernel_destroy(alia_kernel* kernel)
{
    if (!kernel)
        return;

    alia::release_kernel(*kernel);
    kernel->~alia_kernel();
}

void
alia_kernel_mark_dirty(alia_kernel* kernel)
{
    ALIA_ASSERT(kernel);
    kernel->dirty = true;
}

} // extern "C"
