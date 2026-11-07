#pragma once

#include <alia/abi/base/allocator.h>
#include <alia/abi/base/memory.h>
#include <alia/abi/ui/styling/color_program.h>
#include <alia/abi/ui/styling/environment.h>
#include <alia/abi/ui/styling/style_catalog.h>

#include <cstddef>
#include <cstdint>

typedef struct alia_ui_system alia_ui_system;

namespace alia {

struct style_slot_info
{
    uint32_t offset = 0;
    uint16_t size = 0;
    uint8_t align = 1;
    uint8_t bound = 0;
    alia_style_generator_fn generator = nullptr;
    void* user_data = nullptr;
};

struct style_catalog
{
    style_slot_info slots[ALIA_STYLE_SLOT_MAX]{};
    uint32_t slot_count = ALIA_STYLE_BUILTIN_COUNT;
    void* defaults = nullptr;
    size_t defaults_size = 0;
    size_t defaults_capacity = 0;
    // durable color-program step block for default styles
    alia_color_program_step* steps = nullptr;
    uint16_t steps_count = 0;
    alia_general_allocator allocator{};
    alia_memory_pool_id styles_pool = 0;
    alia_ui_system* ui = nullptr;

    style_catalog() = default;

    style_catalog(style_catalog const&) = delete;
    style_catalog&
    operator=(style_catalog const&) = delete;
};

void
style_catalog_init(style_catalog& catalog, alia_ui_system* ui);

// Release the defaults buffer. The catalog stays alive.
void
style_catalog_release(style_catalog& catalog);

void
style_catalog_generate_defaults(
    style_catalog& catalog, alia_style_seeds const* seeds);

} // namespace alia
