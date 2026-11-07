#include <alia/ui/styling/style_catalog.h>

#include <alia/abi/kernel/memory.h>
#include <alia/abi/ui/library.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/memory.h>
#include <alia/ui/system/object.h>

#include <algorithm>
#include <cstring>

namespace {

size_t
align_up(size_t value, size_t alignment)
{
    if (alignment <= 1)
        return value;
    return (value + alignment - 1) & ~(alignment - 1);
}

void
bind_slot(
    alia::style_catalog& catalog,
    uint32_t id,
    size_t size,
    size_t align,
    alia_style_generator_fn generator,
    void* user_data)
{
    ALIA_ASSERT(id < ALIA_STYLE_SLOT_MAX);
    ALIA_ASSERT(generator);
    ALIA_ASSERT(size > 0 && size <= UINT16_MAX);
    size_t const a = align > 0 ? align : 1;
    ALIA_ASSERT(a <= UINT8_MAX);
    auto& slot = catalog.slots[id];
    slot.size = static_cast<uint16_t>(size);
    slot.align = static_cast<uint8_t>(a);
    slot.generator = generator;
    slot.user_data = user_data;
    slot.bound = 1;
}

void
generate_toggle_switch_style(
    void* user_data,
    void* out,
    alia_style_seeds const* seeds,
    alia_color_program_emitter* emitter)
{
    (void) user_data;
    alia_toggle_switch_style_generate(
        static_cast<alia_toggle_switch_style*>(out), seeds, emitter);
}

void
generate_slider_style(
    void* user_data,
    void* out,
    alia_style_seeds const* seeds,
    alia_color_program_emitter* emitter)
{
    (void) user_data;
    alia_slider_style_generate(
        static_cast<alia_slider_style*>(out), seeds, emitter);
}

void
generate_radio_button_style(
    void* user_data,
    void* out,
    alia_style_seeds const* seeds,
    alia_color_program_emitter* emitter)
{
    (void) user_data;
    alia_radio_button_style_generate(
        static_cast<alia_radio_button_style*>(out), seeds, emitter);
}

void
generate_checkbox_style(
    void* user_data,
    void* out,
    alia_style_seeds const* seeds,
    alia_color_program_emitter* emitter)
{
    // `user_data` is the owning `alia_ui_system` (see style_catalog_init).
    alia_checkbox_style_generate(
        static_cast<alia_checkbox_style*>(out),
        static_cast<alia_ui_system*>(user_data),
        seeds,
        emitter);
}

void
generate_node_expander_style(
    void* user_data,
    void* out,
    alia_style_seeds const* seeds,
    alia_color_program_emitter* emitter)
{
    (void) user_data;
    alia_node_expander_style_generate(
        static_cast<alia_node_expander_style*>(out), seeds, emitter);
}

void
generate_scrollbar_style(
    void* user_data,
    void* out,
    alia_style_seeds const* seeds,
    alia_color_program_emitter* emitter)
{
    (void) user_data;
    alia_scrollbar_style_generate(
        static_cast<alia_scrollbar_style*>(out), seeds, emitter);
}

void
generate_button_style(
    void* user_data,
    void* out,
    alia_style_seeds const* seeds,
    alia_color_program_emitter* emitter)
{
    (void) user_data;
    alia_button_style_generate(
        static_cast<alia_button_style*>(out), seeds, emitter);
}

void
generate_separator_style(
    void* user_data,
    void* out,
    alia_style_seeds const* seeds,
    alia_color_program_emitter* emitter)
{
    (void) user_data;
    alia_separator_style_generate(
        static_cast<alia_separator_style*>(out), seeds, emitter);
}

} // namespace

namespace alia {

void
style_catalog_init(style_catalog& catalog, alia_ui_system* ui)
{
    ALIA_ASSERT(ui);
    catalog.ui = ui;
    catalog.ui->styling.environment.seeds = alia_style_seeds_default();
    catalog.slot_count = ALIA_STYLE_BUILTIN_COUNT;
    catalog.defaults = nullptr;
    catalog.defaults_size = 0;
    catalog.defaults_capacity = 0;
    catalog.steps = nullptr;
    catalog.steps_count = 0;
    catalog.styles_pool = alia_memory_register_pool(
        alia_ui_memory(ui), ALIA_MEMORY_POOL_GENERAL, 0);
    catalog.allocator
        = alia_memory_bind_general(alia_ui_memory(ui), catalog.styles_pool);

    for (uint32_t i = 0; i < ALIA_STYLE_SLOT_MAX; ++i)
        catalog.slots[i] = style_slot_info{};

    bind_slot(
        catalog,
        ALIA_STYLE_TOGGLE_SWITCH,
        sizeof(alia_toggle_switch_style),
        alignof(alia_toggle_switch_style),
        generate_toggle_switch_style,
        nullptr);
    bind_slot(
        catalog,
        ALIA_STYLE_SLIDER,
        sizeof(alia_slider_style),
        alignof(alia_slider_style),
        generate_slider_style,
        nullptr);
    bind_slot(
        catalog,
        ALIA_STYLE_RADIO_BUTTON,
        sizeof(alia_radio_button_style),
        alignof(alia_radio_button_style),
        generate_radio_button_style,
        nullptr);
    // Pass `ui` so the generator can copy registered icons into the checkbox
    // style defaults.
    bind_slot(
        catalog,
        ALIA_STYLE_CHECKBOX,
        sizeof(alia_checkbox_style),
        alignof(alia_checkbox_style),
        generate_checkbox_style,
        ui);
    bind_slot(
        catalog,
        ALIA_STYLE_NODE_EXPANDER,
        sizeof(alia_node_expander_style),
        alignof(alia_node_expander_style),
        generate_node_expander_style,
        nullptr);
    bind_slot(
        catalog,
        ALIA_STYLE_SCROLLBAR,
        sizeof(alia_scrollbar_style),
        alignof(alia_scrollbar_style),
        generate_scrollbar_style,
        nullptr);
    bind_slot(
        catalog,
        ALIA_STYLE_BUTTON,
        sizeof(alia_button_style),
        alignof(alia_button_style),
        generate_button_style,
        nullptr);
    bind_slot(
        catalog,
        ALIA_STYLE_SEPARATOR,
        sizeof(alia_separator_style),
        alignof(alia_separator_style),
        generate_separator_style,
        nullptr);
}

void
style_catalog_release(style_catalog& catalog)
{
    if (catalog.defaults && catalog.allocator.free)
    {
        catalog.allocator.free(
            catalog.allocator.user_data,
            catalog.defaults,
            catalog.defaults_capacity,
            ALIA_MIN_ALIGN);
    }
    catalog.defaults = nullptr;
    catalog.defaults_size = 0;
    catalog.defaults_capacity = 0;
    if (catalog.steps && catalog.allocator.free)
    {
        catalog.allocator.free(
            catalog.allocator.user_data,
            catalog.steps,
            sizeof(alia_color_program_step) * catalog.steps_count,
            alignof(alia_color_program_step));
    }
    catalog.steps = nullptr;
    catalog.steps_count = 0;
}

void
style_catalog_generate_defaults(
    style_catalog& catalog, alia_style_seeds const* seeds)
{
    ALIA_ASSERT(catalog.ui);
    if (seeds)
        catalog.ui->styling.environment.seeds = *seeds;

    size_t offset = 0;
    size_t max_align = 1;
    for (uint32_t i = 0; i < catalog.slot_count; ++i)
    {
        auto& slot = catalog.slots[i];
        if (!slot.bound)
            continue;
        ALIA_ASSERT(slot.generator);
        max_align = (std::max)(max_align, size_t{slot.align});
        offset = align_up(offset, slot.align);
        ALIA_ASSERT(offset <= UINT32_MAX);
        slot.offset = static_cast<uint32_t>(offset);
        offset += slot.size;
    }
    ALIA_ASSERT(offset <= UINT32_MAX);
    size_t const blob_size
        = align_up(offset, (std::max)(max_align, size_t{ALIA_MIN_ALIGN}));

    if (blob_size > catalog.defaults_capacity)
    {
        if (catalog.defaults && catalog.allocator.free)
        {
            catalog.allocator.free(
                catalog.allocator.user_data,
                catalog.defaults,
                catalog.defaults_capacity,
                ALIA_MIN_ALIGN);
        }
        catalog.defaults = catalog.allocator.alloc(
            catalog.allocator.user_data, blob_size, max_align);
        ALIA_ASSERT(catalog.defaults);
        catalog.defaults_capacity = blob_size;
    }
    catalog.defaults_size = blob_size;
    std::memset(catalog.defaults, 0, blob_size);

    // Phase 1: emit into the kernel scratch bump (idle outside a pass).
    alia_memory_system* memory = alia_ui_memory(catalog.ui);
    alia_memory_pool_id const scratch_pool
        = alia_kernel_pool(&catalog.ui->kernel, ALIA_KERNEL_POOL_SCRATCH);
    alia_bump_allocator scratch = alia_memory_open_bump(memory, scratch_pool);
    alia_arena_marker const session = alia_arena_mark(&scratch);

    uint16_t step_origins[ALIA_STYLE_SLOT_MAX];
    for (uint32_t i = 0; i < ALIA_STYLE_SLOT_MAX; ++i)
        step_origins[i] = 0;

    for (uint32_t i = 0; i < catalog.slot_count; ++i)
    {
        auto& slot = catalog.slots[i];
        if (!slot.bound)
            continue;
        void* dst = static_cast<char*>(catalog.defaults) + slot.offset;
        step_origins[i] = static_cast<uint16_t>(
            (scratch.offset - session.offset)
            / sizeof(alia_color_program_step));
        alia_color_program_emitter emitter
            = alia_color_program_emitter_begin(&scratch);
        slot.generator(
            slot.user_data,
            dst,
            &catalog.ui->styling.environment.seeds,
            &emitter);
    }

    size_t const scratch_bytes = scratch.offset - session.offset;
    ALIA_ASSERT(scratch_bytes % sizeof(alia_color_program_step) == 0);
    uint16_t const scratch_count = static_cast<uint16_t>(
        scratch_bytes / sizeof(alia_color_program_step));
    alia_color_program_step const* scratch_steps
        = scratch_bytes == 0
              ? nullptr
              : static_cast<alia_color_program_step const*>(
                    alia_arena_ptr(&scratch, session.offset));

    // Phase 2: exact durable steps block in the styles pool.
    if (catalog.steps && catalog.allocator.free)
    {
        catalog.allocator.free(
            catalog.allocator.user_data,
            catalog.steps,
            sizeof(alia_color_program_step) * catalog.steps_count,
            alignof(alia_color_program_step));
    }
    catalog.steps = nullptr;
    catalog.steps_count = 0;

    if (scratch_count > 0)
    {
        size_t const bytes
            = sizeof(alia_color_program_step) * scratch_count;
        catalog.steps = static_cast<alia_color_program_step*>(
            catalog.allocator.alloc(
                catalog.allocator.user_data,
                bytes,
                alignof(alia_color_program_step)));
        ALIA_ASSERT(catalog.steps);
        std::memcpy(catalog.steps, scratch_steps, bytes);
        catalog.steps_count = scratch_count;
    }

    auto retarget_steps = [&](uint32_t slot, auto* style) {
        if (!catalog.slots[slot].bound)
            return;
        style->steps
            = catalog.steps ? catalog.steps + step_origins[slot] : nullptr;
    };

    if (catalog.slots[ALIA_STYLE_TOGGLE_SWITCH].bound)
    {
        retarget_steps(
            ALIA_STYLE_TOGGLE_SWITCH,
            reinterpret_cast<alia_toggle_switch_style*>(
                static_cast<char*>(catalog.defaults)
                + catalog.slots[ALIA_STYLE_TOGGLE_SWITCH].offset));
    }
    if (catalog.slots[ALIA_STYLE_SLIDER].bound)
    {
        retarget_steps(
            ALIA_STYLE_SLIDER,
            reinterpret_cast<alia_slider_style*>(
                static_cast<char*>(catalog.defaults)
                + catalog.slots[ALIA_STYLE_SLIDER].offset));
    }
    if (catalog.slots[ALIA_STYLE_RADIO_BUTTON].bound)
    {
        retarget_steps(
            ALIA_STYLE_RADIO_BUTTON,
            reinterpret_cast<alia_radio_button_style*>(
                static_cast<char*>(catalog.defaults)
                + catalog.slots[ALIA_STYLE_RADIO_BUTTON].offset));
    }
    if (catalog.slots[ALIA_STYLE_CHECKBOX].bound)
    {
        retarget_steps(
            ALIA_STYLE_CHECKBOX,
            reinterpret_cast<alia_checkbox_style*>(
                static_cast<char*>(catalog.defaults)
                + catalog.slots[ALIA_STYLE_CHECKBOX].offset));
    }
    if (catalog.slots[ALIA_STYLE_NODE_EXPANDER].bound)
    {
        retarget_steps(
            ALIA_STYLE_NODE_EXPANDER,
            reinterpret_cast<alia_node_expander_style*>(
                static_cast<char*>(catalog.defaults)
                + catalog.slots[ALIA_STYLE_NODE_EXPANDER].offset));
    }
    if (catalog.slots[ALIA_STYLE_SCROLLBAR].bound)
    {
        retarget_steps(
            ALIA_STYLE_SCROLLBAR,
            reinterpret_cast<alia_scrollbar_style*>(
                static_cast<char*>(catalog.defaults)
                + catalog.slots[ALIA_STYLE_SCROLLBAR].offset));
    }
    if (catalog.slots[ALIA_STYLE_BUTTON].bound)
    {
        retarget_steps(
            ALIA_STYLE_BUTTON,
            reinterpret_cast<alia_button_style*>(
                static_cast<char*>(catalog.defaults)
                + catalog.slots[ALIA_STYLE_BUTTON].offset));
    }
    if (catalog.slots[ALIA_STYLE_SEPARATOR].bound)
    {
        retarget_steps(
            ALIA_STYLE_SEPARATOR,
            reinterpret_cast<alia_separator_style*>(
                static_cast<char*>(catalog.defaults)
                + catalog.slots[ALIA_STYLE_SEPARATOR].offset));
    }

    alia_memory_discard_bump(memory, scratch_pool);
}

} // namespace alia

extern "C" {

alia_style_slot_id
alia_style_slot_add(
    alia_ui_system* ui,
    size_t size,
    size_t align,
    alia_style_generator_fn generator,
    void* user_data)
{
    ALIA_ASSERT(ui);
    auto& catalog = ui->styling.styles;
    ALIA_ASSERT(catalog.slot_count < ALIA_STYLE_SLOT_MAX);
    alia_style_slot_id const id = catalog.slot_count++;
    bind_slot(catalog, id, size, align, generator, user_data);
    return id;
}

void
alia_style_slot_set_generator(
    alia_ui_system* ui,
    alia_style_slot_id slot,
    alia_style_generator_fn generator,
    void* user_data)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(slot < ui->styling.styles.slot_count);
    ALIA_ASSERT(ui->styling.styles.slots[slot].bound);
    ALIA_ASSERT(generator);
    ui->styling.styles.slots[slot].generator = generator;
    ui->styling.styles.slots[slot].user_data = user_data;
}

void
alia_style_generate_defaults(alia_ui_system* ui, alia_style_seeds const* seeds)
{
    ALIA_ASSERT(ui);
    alia::style_catalog_generate_defaults(ui->styling.styles, seeds);
    alia_ui_metrics_changed(ui);
}

void*
alia_style_default(alia_ui_system* ui, alia_style_slot_id slot)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(slot < ui->styling.styles.slot_count);
    ALIA_ASSERT(ui->styling.styles.slots[slot].bound);
    ALIA_ASSERT(ui->styling.styles.defaults);
    return static_cast<char*>(ui->styling.styles.defaults)
         + ui->styling.styles.slots[slot].offset;
}

void*
alia_style_active(alia_ui_context* ctx, alia_style_slot_id slot)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(alia_ctx_system(ctx));
    ALIA_ASSERT(alia_ctx_style(ctx)->active_styles);
    auto const& catalog = alia_ctx_system(ctx)->styling.styles;
    ALIA_ASSERT(slot < catalog.slot_count);
    ALIA_ASSERT(catalog.slots[slot].bound);
    return static_cast<char*>(alia_ctx_style(ctx)->active_styles)
         + catalog.slots[slot].offset;
}

} // extern "C"
