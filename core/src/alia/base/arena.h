#pragma once

#include <alia/abi/base/arena.h>

#include <alia/prelude.hpp>

// ABI STRUCTURES

struct alia_arena
{
    uint8_t* base = nullptr;
    size_t capacity = 0;
    alia_arena_controller controller{};
    size_t peak_usage = 0;
};
