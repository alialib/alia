#include <alia/ui/styling.h>

#include <alia/abi/ui/library.h>
#include <alia/abi/ui/text.h>
#include <alia/ui/system/object.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace {

size_t
align_up(size_t value, size_t alignment)
{
    if (alignment <= 1)
        return value;
    return (value + alignment - 1) & ~(alignment - 1);
}

void*
style_block_alloc(void* user_data, size_t size, size_t alignment)
{
    (void) user_data;
    (void) alignment;
    return std::malloc(size);
}

void
style_block_free(void* user_data, void* ptr, size_t size, size_t alignment)
{
    (void) user_data;
    (void) size;
    (void) alignment;
    std::free(ptr);
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
generate_layout_style(
    void* user_data, void* out, alia_style_seeds const* seeds)
{
    (void) user_data;
    alia_layout_style_generate(static_cast<alia_layout_style*>(out), seeds);
}

void
generate_text_style(void* user_data, void* out, alia_style_seeds const* seeds)
{
    (void) user_data;
    alia_text_style_generate(static_cast<alia_text_style*>(out), seeds);
}

void
generate_focus_style(void* user_data, void* out, alia_style_seeds const* seeds)
{
    (void) user_data;
    alia_focus_style_generate(static_cast<alia_focus_style*>(out), seeds);
}

void
generate_switch_style(
    void* user_data, void* out, alia_style_seeds const* seeds)
{
    (void) user_data;
    alia_switch_style_generate(static_cast<alia_switch_style*>(out), seeds);
}

void
generate_slider_style(
    void* user_data, void* out, alia_style_seeds const* seeds)
{
    (void) user_data;
    alia_slider_style_generate(static_cast<alia_slider_style*>(out), seeds);
}

void
generate_radio_style(void* user_data, void* out, alia_style_seeds const* seeds)
{
    (void) user_data;
    alia_radio_style_generate(static_cast<alia_radio_style*>(out), seeds);
}

void
generate_checkbox_style(
    void* user_data, void* out, alia_style_seeds const* seeds)
{
    (void) user_data;
    alia_checkbox_style_generate(
        static_cast<alia_checkbox_style*>(out), seeds);
}

void
generate_node_expander_style(
    void* user_data, void* out, alia_style_seeds const* seeds)
{
    (void) user_data;
    alia_node_expander_style_generate(
        static_cast<alia_node_expander_style*>(out), seeds);
}

void
generate_scrollbar_style(
    void* user_data, void* out, alia_style_seeds const* seeds)
{
    (void) user_data;
    alia_scrollbar_style_generate(
        static_cast<alia_scrollbar_style*>(out), seeds);
}

} // namespace

namespace alia {

void
style_catalog_init(style_catalog& catalog)
{
    catalog.seeds = alia_style_seeds_default();
    catalog.slot_count = ALIA_STYLE_BUILTIN_COUNT;
    catalog.defaults = nullptr;
    catalog.defaults_size = 0;
    catalog.defaults_capacity = 0;
    catalog.allocator.user_data = nullptr;
    catalog.allocator.alloc = style_block_alloc;
    catalog.allocator.free = style_block_free;

    for (uint32_t i = 0; i < ALIA_STYLE_SLOT_MAX; ++i)
        catalog.slots[i] = style_slot_info{};

    bind_slot(
        catalog,
        ALIA_STYLE_LAYOUT,
        sizeof(alia_layout_style),
        alignof(alia_layout_style),
        generate_layout_style,
        nullptr);
    bind_slot(
        catalog,
        ALIA_STYLE_TEXT,
        sizeof(alia_text_style),
        alignof(alia_text_style),
        generate_text_style,
        nullptr);
    bind_slot(
        catalog,
        ALIA_STYLE_FOCUS,
        sizeof(alia_focus_style),
        alignof(alia_focus_style),
        generate_focus_style,
        nullptr);
    bind_slot(
        catalog,
        ALIA_STYLE_SWITCH,
        sizeof(alia_switch_style),
        alignof(alia_switch_style),
        generate_switch_style,
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
        ALIA_STYLE_RADIO,
        sizeof(alia_radio_style),
        alignof(alia_radio_style),
        generate_radio_style,
        nullptr);
    bind_slot(
        catalog,
        ALIA_STYLE_CHECKBOX,
        sizeof(alia_checkbox_style),
        alignof(alia_checkbox_style),
        generate_checkbox_style,
        nullptr);
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
}

style_catalog::~style_catalog()
{
    style_catalog_destroy(*this);
}

void
style_catalog_destroy(style_catalog& catalog)
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
}

void
style_catalog_generate_defaults(
    style_catalog& catalog, alia_style_seeds const* seeds)
{
    if (seeds)
        catalog.seeds = *seeds;

    size_t offset = 0;
    size_t max_align = 1;
    for (uint32_t i = 0; i < catalog.slot_count; ++i)
    {
        auto& slot = catalog.slots[i];
        if (!slot.bound)
            continue;
        ALIA_ASSERT(slot.generator);
        max_align = (std::max) (max_align, size_t{slot.align});
        offset = align_up(offset, slot.align);
        ALIA_ASSERT(offset <= UINT32_MAX);
        slot.offset = static_cast<uint32_t>(offset);
        offset += slot.size;
    }
    ALIA_ASSERT(offset <= UINT32_MAX);
    size_t const blob_size
        = align_up(offset, (std::max) (max_align, size_t{ALIA_MIN_ALIGN}));

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

    for (uint32_t i = 0; i < catalog.slot_count; ++i)
    {
        auto& slot = catalog.slots[i];
        if (!slot.bound)
            continue;
        void* dst = static_cast<char*>(catalog.defaults) + slot.offset;
        slot.generator(slot.user_data, dst, &catalog.seeds);
    }
}

} // namespace alia

extern "C" {

alia_style_seeds
alia_style_seeds_default(void)
{
    return alia_style_seeds{.spacing = 10.f, .scale = 1.f, .roundness = 1.f};
}

void
alia_layout_style_generate(
    alia_layout_style* out, alia_style_seeds const* seeds)
{
    ALIA_ASSERT(out);
    alia_style_seeds const s = seeds ? *seeds : alia_style_seeds_default();
    out->spacing = s.spacing;
}

void
alia_focus_style_generate(alia_focus_style* out, alia_style_seeds const* seeds)
{
    ALIA_ASSERT(out);
    alia_style_seeds const s = seeds ? *seeds : alia_style_seeds_default();
    float const scale = s.scale;
    out->color = alia_palette_color_make(
        alia_palette_index_swatch(
            ALIA_PALETTE_SWATCH_FOCUS, ALIA_PALETTE_SWATCH_PART_OUTLINE),
        0xff);
    out->outset = 2.f * scale;
    out->thickness = 2.f * scale;
    // Use a soft radius at roundness 1 and a square corner at 0.
    out->corner_radius = 4.f * scale * s.roundness;
    out->show_on_pointer_focus = false;
}

alia_style_slot_id
alia_style_slot_add(
    alia_ui_system* ui,
    size_t size,
    size_t align,
    alia_style_generator_fn generator,
    void* user_data)
{
    ALIA_ASSERT(ui);
    auto& catalog = ui->styles;
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
    ALIA_ASSERT(slot < ui->styles.slot_count);
    ALIA_ASSERT(ui->styles.slots[slot].bound);
    ALIA_ASSERT(generator);
    ui->styles.slots[slot].generator = generator;
    ui->styles.slots[slot].user_data = user_data;
}

void
alia_style_generate_defaults(alia_ui_system* ui, alia_style_seeds const* seeds)
{
    ALIA_ASSERT(ui);
    alia::style_catalog_generate_defaults(ui->styles, seeds);
}

void*
alia_style_default(alia_ui_system* ui, alia_style_slot_id slot)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(slot < ui->styles.slot_count);
    ALIA_ASSERT(ui->styles.slots[slot].bound);
    ALIA_ASSERT(ui->styles.defaults);
    return static_cast<char*>(ui->styles.defaults)
         + ui->styles.slots[slot].offset;
}

void*
alia_style_active(alia_context* ctx, alia_style_slot_id slot)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(ctx->system);
    ALIA_ASSERT(ctx->active_styles);
    auto const& catalog = ctx->system->styles;
    ALIA_ASSERT(slot < catalog.slot_count);
    ALIA_ASSERT(catalog.slots[slot].bound);
    return static_cast<char*>(ctx->active_styles) + catalog.slots[slot].offset;
}

} // extern "C"
