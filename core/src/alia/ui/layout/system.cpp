#include <alia/ui/layout/system.h>

#include <alia/abi/ui/layout/placement.h>
#include <alia/impl/base/arena.hpp>

using namespace alia;

extern "C" {

void
alia_layout_system_init(alia_layout_system* system)
{
    initialize_lazy_commit_arena(&system->node_arena);
    initialize_lazy_commit_arena(&system->scratch_arena);
    system->root = alia_layout_container{
        .base = {.vtable = nullptr, .next_sibling = nullptr},
        .flags = 0,
        .first_child = 0};
    initialize_lazy_commit_arena(&system->placement.arena);
    system->placement.generation = 0;
    system->placement.high_water = 0;
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
