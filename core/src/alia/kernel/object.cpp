#include <alia/kernel/object.h>

#include <alia/abi/kernel/memory.h>
#include <alia/abi/panic.h>
#include <alia/base/memory.h>
#include <alia/kernel/substrate.h>

#include <new>

namespace alia {

void
init_kernel(alia_kernel& kernel, alia_memory_ops memory_ops)
{
    alia_memory_system_init(&kernel.memory, memory_ops);
    kernel.stack_pool = alia_memory_register_pool(
        &kernel.memory, ALIA_MEMORY_POOL_STACK, ALIA_MEMORY_RELEASE_ON_IDLE);
    kernel.scratch_pool = alia_memory_register_pool(
        &kernel.memory, ALIA_MEMORY_POOL_BUMP, ALIA_MEMORY_RELEASE_ON_IDLE);
    kernel.substrate_discovery_pool = alia_memory_register_pool(
        &kernel.memory, ALIA_MEMORY_POOL_BUMP, ALIA_MEMORY_RELEASE_ON_IDLE);
    kernel.substrate_storage_pool = alia_memory_register_pool(
        &kernel.memory, ALIA_MEMORY_POOL_GENERAL, 0);

    alia_general_allocator allocator = alia_memory_bind_general(
        &kernel.memory, kernel.substrate_storage_pool);
    substrate_system_init(kernel.substrate, allocator);

    kernel.dirty = true;
}

void
release_kernel(alia_kernel& kernel)
{
    substrate_system_destroy(kernel.substrate);
    alia_memory_system_destroy(&kernel.memory);
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
alia_kernel_init(void* object_storage, alia_memory_ops memory_ops)
{
    ALIA_ASSERT(object_storage);

    alia_kernel* kernel = new (object_storage) alia_kernel;
    alia::init_kernel(*kernel, memory_ops);
    return kernel;
}

alia_memory_system*
alia_kernel_memory(alia_kernel* kernel)
{
    ALIA_ASSERT(kernel);
    return &kernel->memory;
}

alia_memory_pool_id
alia_kernel_pool(alia_kernel* kernel, alia_kernel_pool_name name)
{
    ALIA_ASSERT(kernel);
    switch (name)
    {
        case ALIA_KERNEL_POOL_STACK:
            return kernel->stack_pool;
        case ALIA_KERNEL_POOL_SCRATCH:
            return kernel->scratch_pool;
        case ALIA_KERNEL_POOL_SUBSTRATE_DISCOVERY:
            return kernel->substrate_discovery_pool;
        case ALIA_KERNEL_POOL_SUBSTRATE_STORAGE:
            return kernel->substrate_storage_pool;
    }
    alia_panic_info info = {
        .reason = ALIA_PANIC_API_MISUSE,
        .subsystem = "memory",
        .msg = "kernel pool name",
    };
    alia_panic_now(&info);
    return 0;
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
