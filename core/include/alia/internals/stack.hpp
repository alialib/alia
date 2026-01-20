#pragma once

#include <alia/abi/stack.h>

#include <alia/base.hpp>

// ABI STRUCTURES

struct alia_stack
{
    uint8_t* base = nullptr;
    size_t capacity = 0;
    // write cursor in bytes from base (always aligned to ALIA_MIN_ALIGN)
    uint32_t top = 0;
    // size of top entry in bytes (0 if stack is empty)
    uint16_t top_entry_size = 0;
};
