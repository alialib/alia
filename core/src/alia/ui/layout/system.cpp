#include <alia/ui/layout/system.h>

#include <alia/abi/ui/layout/placement.h>
#include <alia/abi/ui/system/memory.h>

using namespace alia;

extern "C" {

void
alia_layout_system_init(alia_layout_system* system, alia_memory_system* memory)
{
    ALIA_ASSERT(system);
    ALIA_ASSERT(memory);

    system->memory = memory;
    system->root = alia_layout_container{
        .base = {.vtable = nullptr, .next_sibling = nullptr},
        .flags = 0,
        .first_child = 0};
    system->placement.generation = 0;
    system->placement.high_water = 0;

    system->node_pool
        = alia_memory_register_pool(memory, ALIA_MEMORY_POOL_BUMP, 0);
    system->scratch_pool = alia_memory_register_pool(
        memory, ALIA_MEMORY_POOL_BUMP, ALIA_MEMORY_RELEASE_ON_IDLE);
    system->placement_pool
        = alia_memory_register_pool(memory, ALIA_MEMORY_POOL_BUMP, 0);
}

void
alia_layout_system_resolve(
    alia_layout_system* system, alia_vec2f available_space)
{
    alia_layout_placement_clear(system);

    alia_layout_node* root_node = system->root.first_child;
    if (!root_node)
        return;

    alia_layout_placement_resolve(system, root_node, available_space);
}

} // extern "C"
