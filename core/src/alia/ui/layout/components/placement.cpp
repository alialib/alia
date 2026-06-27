#include <alia/abi/ui/layout/api.h>

#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>

using namespace alia;

extern "C" {

alia_box
alia_layout_consume_box(alia_context* ctx)
{
    return *arena_alloc<alia_box>(*alia_layout_placement_arena(ctx));
}

alia_layout_box_array
alia_layout_consume_box_array(alia_context* ctx)
{
    uint32_t* count = arena_alloc<uint32_t>(*alia_layout_placement_arena(ctx));
    alia_box* boxes = arena_alloc_array<alia_box>(
        *alia_layout_placement_arena(ctx), *count);
    return {.count = *count, .boxes = boxes};
}

} // extern "C"
