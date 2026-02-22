#include <alia/system/object.hpp>

#include <alia/abi/ui/layout/system.h>
#include <alia/substrate.hpp>

#include <alia/impl/base/arena.hpp>

namespace alia {

// TODO: Sort this out.
void*
allocate_virtual_block(std::size_t size);

// TODO: Sort this out too.
static void*
block_alloc(void* user, size_t size, size_t alignment)
{
    return malloc(size);
}
static void
block_free(void* user, void* ptr)
{
    free(ptr);
}

void
initialize_ui_system(alia_ui_system* system, alia_vec2f surface_size)
{
    // TODO: Sort this out.
    void* block = allocate_virtual_block(1024 * 1024);
    alia_stack_init(&system->stack, block, 1024 * 1024);

    alia_layout_system_init(&system->layout);
    system->surface_size = surface_size;

    // TODO: Sort this out.
    alia::initialize_lazy_commit_arena(&system->substrate_discovery_arena);

    alia_substrate_allocator allocator;
    allocator.user_data = nullptr;
    allocator.alloc = block_alloc;
    allocator.free = block_free;
    construct_substrate_system(system->substrate, allocator);
}

// TODO: cleanup function

} // namespace alia
