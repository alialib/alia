#include <alia/system/object.hpp>

#include <alia/abi/ui/layout/system.h>

namespace alia {

// TODO: Sort this out.
void*
allocate_virtual_block(std::size_t size);

void
initialize_ui_system(alia_ui_system* system, alia_vec2f surface_size)
{
    // TODO: Sort this out.
    void* block = allocate_virtual_block(1024 * 1024);
    alia_stack_init(&system->stack, block, 1024 * 1024);

    alia_layout_system_init(&system->layout);
    system->surface_size = surface_size;
}

// TODO: cleanup function

} // namespace alia
