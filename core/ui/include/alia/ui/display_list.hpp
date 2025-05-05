#pragma once

#include <cstdint>

#include <alia/foundation/arena.hpp>
#include <alia/ui/color.hpp>
#include <alia/ui/geometry.hpp>

namespace alia {

enum class DrawCommandType : std::uint8_t
{
    Box
};

struct DrawCommand
{
    DrawCommandType type;
    Box box;
    Color color;
};

struct DisplayList
{
    Arena* arena;
    DrawCommand* commands;
    size_t count;
};

DisplayList
create_display_list(Arena* arena);

void
reset_display_list(DisplayList* display_list);

void
destroy_display_list(DisplayList* display_list);

} // namespace alia
