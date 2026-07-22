#include <alia/abi/ui/text.h>

#include <alia/abi/kernel/ids.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/geometry.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/layout/protocol.h>
#include <alia/abi/ui/layout/utilities/emission.h>
#include <alia/abi/ui/layout/utilities/flow.h>
#include <alia/abi/ui/layout/utilities/placement.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/styling.h>

#include <alia/impl/base/arena.hpp>
#include <alia/impl/events.hpp>

namespace alia {

// The text layout node is rebuilt in the layout emission arena on every
// refresh. It caches (by value) everything the layout vtable needs so that the
// measure/place callbacks - which only receive a measurement/placement context
// - never have to reach back for the active font or geometry scale. The
// prepared `block` and `engine` are borrowed from the per-call-site substrate
// cache (see `text_block_cache`), which outlives any single layout resolution.
struct text_layout_node
{
    alia_layout_node base;
    alia_layout_flags_t flags;
    // fixed edge spacing (matches the leaf layout convention)
    float spacing;
    alia_text_engine* engine;
    alia_text_block* block;
    // total byte length of the prepared text
    size_t text_length;
    // font size in physical pixels (logical size * geometry scale)
    float font_size;
    // line metrics in physical pixels
    float line_height;
    float ascender;
    float descender;
};

// Placement records are written into the placement arena during layout
// resolution (leaf `assign_boxes` or flow `read_fragment_placements`) and read
// back by the component on its non-refresh passes. One header precedes a run
// of per-piece fragments.
struct text_placement_header
{
    int fragment_count;
};

struct text_placement_fragment
{
    // the piece's box in layout space (for hit testing)
    alia_box box;
    // the baseline pen origin in layout space (for drawing)
    alia_vec2f baseline_origin;
    // source byte range within the block
    size_t byte_start;
    size_t byte_end;
};

static float
text_measure_total_width(text_layout_node const& node)
{
    alia_text_engine* engine = node.engine;
    int const count = engine->vtable->segment_count(engine, node.block);
    float total = 0.f;
    for (int i = 0; i < count; ++i)
    {
        alia_text_segment seg;
        engine->vtable->segment_info(engine, node.block, i, &seg);
        total += seg.advance_width;
    }
    return total;
}

static float
text_effective_spacing(text_layout_node const& node)
{
    return (node.flags & ALIA_FLUSH) != 0 ? 0.f : node.spacing;
}

// LEAF / WHOLE-STRING PROTOCOL

static alia_horizontal_requirements
text_measure_horizontal(alia_measurement_context* ctx, alia_layout_node* node)
{
    (void) ctx;
    auto& text = *reinterpret_cast<text_layout_node*>(node);
    float const spacing = text_effective_spacing(text);
    return alia_horizontal_requirements{
        .min_size = text_measure_total_width(text) + spacing * 2,
        .growth_factor = alia_resolve_growth_factor(text.flags)};
}

static alia_vertical_requirements
text_measure_vertical(
    alia_measurement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    float assigned_width)
{
    (void) ctx;
    (void) main_axis;
    (void) assigned_width;
    auto& text = *reinterpret_cast<text_layout_node*>(node);
    float const spacing = text_effective_spacing(text);
    return alia_vertical_requirements{
        .min_size = text.line_height + spacing * 2,
        .growth_factor = alia_resolve_growth_factor(text.flags),
        .ascent = text.ascender,
        .descent = -text.descender};
}

static void
text_assign_boxes(
    alia_placement_context* ctx,
    alia_main_axis_index main_axis,
    alia_layout_node* node,
    alia_box box,
    float baseline)
{
    auto& text = *reinterpret_cast<text_layout_node*>(node);
    float const spacing = text_effective_spacing(text);
    float const width = text_measure_total_width(text);

    alia_box const placement = alia_resolve_leaf_box(
        alia_fold_in_cross_axis_flags(text.flags, main_axis),
        box.size,
        baseline,
        alia_vec2f{width, text.line_height},
        text.ascender,
        alia_vec2f{spacing, spacing});

    alia_box const resolved{
        alia_vec2f_add(box.min, placement.min), placement.size};

    auto* header = arena_alloc<text_placement_header>(ctx->arena);
    header->fragment_count = 1;
    auto* frag = arena_alloc<text_placement_fragment>(ctx->arena);
    frag->box = resolved;
    frag->baseline_origin
        = alia_vec2f{resolved.min.x, resolved.min.y + text.ascender};
    frag->byte_start = 0;
    frag->byte_end = text.text_length;
}

// FLOW PROTOCOL

static alia_flow_emission_counts
text_count_flow_emissions(
    alia_measurement_context* ctx, alia_layout_node* node)
{
    (void) ctx;
    auto& text = *reinterpret_cast<text_layout_node*>(node);
    int const count
        = text.engine->vtable->segment_count(text.engine, text.block);
    return alia_flow_emission_counts_with_run_scope(
        alia_flow_emission_counts{.fragment_count = count});
}

static void
text_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter)
{
    (void) ctx;
    auto& text = *reinterpret_cast<text_layout_node*>(node);
    alia_text_engine* engine = text.engine;

    // A run scope carries this node's horizontal edge spacing into the flow.
    alia_flow_emit_run_push_control(
        emitter,
        alia_edge_offsets{.left = text.spacing, .right = text.spacing});

    int const count = engine->vtable->segment_count(engine, text.block);
    for (int i = 0; i < count; ++i)
    {
        alia_text_segment seg;
        engine->vtable->segment_info(engine, text.block, i, &seg);

        alia_line_requirements const line
            = alia_layout_line_requirements_with_run_offsets(
                emitter,
                alia_line_requirements{
                    .height = text.line_height,
                    .ascent = text.ascender,
                    .descent = -text.descender});

        alia_flow_fragment fragment;
        fragment.kind = ALIA_FLOW_FRAGMENT_KIND_CONTENT;
        fragment.content = alia_layout_content_metrics{
            .size = alia_vec2f{seg.advance_width, line.height},
            .ascent = line.ascent,
            .descent = line.descent};

        switch (seg.kind)
        {
            case ALIA_TEXT_SEGMENT_SPACE:
                fragment.flags = ALIA_FLOW_FRAGMENT_SUPPRESS_AT_LINE_EDGES;
                break;
            case ALIA_TEXT_SEGMENT_HARD_BREAK:
                fragment.flags = ALIA_FLOW_FRAGMENT_BREAK_AFTER
                               | ALIA_FLOW_FRAGMENT_OMIT_FROM_BOUNDS;
                break;
            case ALIA_TEXT_SEGMENT_CONTENT:
            default:
                fragment.flags = 0;
                break;
        }

        alia_layout_emit_flow_fragment(emitter, fragment);
    }

    alia_flow_emit_run_pop_control(emitter);
}

static void
text_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader)
{
    auto& text = *reinterpret_cast<text_layout_node*>(node);
    alia_text_engine* engine = text.engine;

    alia_layout_skip_flow_run_push_fragment(reader);

    auto* header = arena_alloc<text_placement_header>(ctx->arena);
    header->fragment_count = 0;

    int const count = engine->vtable->segment_count(engine, text.block);
    for (int i = 0; i < count; ++i)
    {
        alia_text_segment seg;
        engine->vtable->segment_info(engine, text.block, i, &seg);

        alia_flow_fragment const* spec
            = alia_layout_read_fragment_spec(reader);
        alia_flow_fragment_placement const* placement
            = alia_layout_read_fragment_placement(reader);
        alia_layout_advance_fragment(reader);

        bool const suppressed
            = (placement->flags & ALIA_FLOW_FRAGMENT_PLACEMENT_SUPPRESSED)
           != 0;
        if (seg.kind != ALIA_TEXT_SEGMENT_CONTENT || suppressed)
            continue;

        float const width = spec->content.size.x;
        alia_vec2f const top_left{
            placement->position.x,
            placement->position.y + placement->baseline - text.ascender};

        auto* frag = arena_alloc<text_placement_fragment>(ctx->arena);
        frag->box = alia_box{top_left, alia_vec2f{width, text.line_height}};
        frag->baseline_origin = alia_vec2f{
            placement->position.x,
            placement->position.y + placement->baseline};
        frag->byte_start = seg.byte_start;
        frag->byte_end = seg.byte_end;
        ++header->fragment_count;
    }

    alia_layout_skip_flow_run_pop_fragment(reader);
}

static alia_layout_node_vtable text_layout_vtable
    = {text_measure_horizontal,
       text_measure_vertical,
       text_assign_boxes,
       text_count_flow_emissions,
       text_emit_flow_fragments,
       text_read_fragment_placements};

// PER-CALL-SITE BLOCK CACHE (Tier B)

// One of these lives in the substrate at each `alia_text` call site. It holds
// the engine's prepared block across frames and the keys that determine when
// it must be rebuilt: the content (`value_id`), the typeface
// (`engine_handle`), and the physical size (which folds in the geometry scale,
// since the block is prepared - and drawn - in physical pixels).
struct text_block_cache
{
    alia_text_engine* engine;
    alia_text_block* block;
    void* engine_handle;
    float physical_size;
    // resolved byte length of the prepared text (a null-terminated signal's
    // length is computed here, once, when the block is prepared)
    size_t text_length;
    alia_captured_id value_id;
};

static void
text_block_cache_cleanup(
    alia_substrate_system*, void* payload, alia_substrate_cleanup_mode)
{
    auto* cache = reinterpret_cast<text_block_cache*>(payload);
    if (cache->block && cache->engine)
    {
        cache->engine->vtable->release_block(cache->engine, cache->block);
        cache->block = nullptr;
    }
    alia_captured_id_release(&cache->value_id);
}

} // namespace alia

using namespace alia;

ALIA_EXTERN_C_BEGIN

void
alia_text_style_generate(alia_text_style* out, alia_style_seeds const* seeds)
{
    (void) seeds;
    *out = alia_text_style{
        .font = nullptr,
        .color = alia_palette_color_make(
            alia_palette_index_foundation_ramp(
                ALIA_PALETTE_FOUNDATION_RAMP_TEXT,
                ALIA_PALETTE_RAMP_LEVEL_BASE),
            0xff),
    };
}

void
alia_text(
    alia_context* ctx,
    alia_layout_flags_t flags,
    alia_text_input_signal text,
    alia_text_style const* style)
{
    alia_text_style const* const effective_style
        = style != nullptr ? style : alia_text_style_active(ctx);

    alia_substrate_usage_result const result = alia_substrate_use_object(
        ctx,
        sizeof(text_block_cache),
        alignof(text_block_cache),
        text_block_cache_cleanup);
    auto* cache = reinterpret_cast<text_block_cache*>(result.ptr);
    bool const fresh = result.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;
    if (fresh)
        *cache = text_block_cache{};

    alia_element_id const id = alia_make_element_id(ctx, result);

    alia_resolved_font const* font
        = effective_style->font != nullptr ? effective_style->font
                                           : alia_active_font(ctx);
    float const geometry_scale = ctx->geometry->scale;
    float const physical_size = font->size * geometry_scale;
    alia_text_engine* engine = font->typeface.engine;
    void* engine_handle = font->typeface.engine_handle;

    // (Re)prepare the block when the content, typeface, or physical size
    // changes. On a fresh slot the previous state is undefined, so we always
    // rebuild without touching it.
    bool const changed
        = fresh || cache->engine_handle != engine_handle
       || cache->physical_size != physical_size
       || !alia_captured_id_matches_view(&cache->value_id, text.value_id);
    if (changed)
    {
        if (cache->block && cache->engine)
            cache->engine->vtable->release_block(cache->engine, cache->block);
        alia_captured_id_release(&cache->value_id);

        // Only inline value IDs (u64 revisions, literal pointers) are
        // supported for now; external/content-hashed IDs would need a
        // variable-size capture slab.
        alia_struct_spec const spec = alia_captured_id_spec(text.value_id);
        ALIA_ASSERT(
            spec.size <= sizeof(cache->value_id)
            && spec.align <= alignof(alia_captured_id));
        alia_captured_id_capture_into(
            text.value_id, &cache->value_id, sizeof(cache->value_id));

        // Resolve the length lazily: this is the only path that scans the
        // bytes of a null-terminated (literal) signal, and it runs only when
        // the content actually changes.
        size_t const length = text.length == ALIA_TEXT_LENGTH_NULL_TERMINATED
                                ? (text.text ? strlen(text.text) : 0)
                                : text.length;

        cache->engine = engine;
        cache->engine_handle = engine_handle;
        cache->physical_size = physical_size;
        cache->text_length = length;
        cache->block = engine->vtable->prepare_block(
            engine,
            engine_handle,
            physical_size,
            ALIA_TEXT_DIRECTION_LTR,
            text.text,
            length);
    }

    alia_event_category const category = get_event_category(*ctx);
    if (category == ALIA_CATEGORY_REFRESH)
    {
        auto& emission = ctx->layout->emission;
        auto* node = arena_alloc<text_layout_node>(emission.arena);
        *emission.next_ptr = &node->base;
        emission.next_ptr = &node->base.next_sibling;
        *node = text_layout_node{
            .base = {.vtable = &text_layout_vtable, .next_sibling = nullptr},
            .flags = flags,
            .spacing = alia_layout_style_active(ctx)->spacing,
            .engine = engine,
            .block = cache->block,
            .text_length = cache->text_length,
            .font_size = physical_size,
            .line_height = font->metrics.line_height * geometry_scale,
            .ascender = font->metrics.ascender * geometry_scale,
            .descender = font->metrics.descender * geometry_scale};
        return;
    }

    // Non-refresh passes: replay the placement records the layout wrote (this
    // must happen on every pass to keep the placement arena walk in sync) and
    // act on them per category.
    alia_bump_allocator* arena = alia_layout_placement_arena(ctx);
    auto const* header = arena_alloc<text_placement_header>(*arena);
    alia_z_index const z = ctx->geometry->z_base;
    for (int i = 0; i < header->fragment_count; ++i)
    {
        auto const* frag = arena_alloc<text_placement_fragment>(*arena);
        switch (category)
        {
            case ALIA_CATEGORY_SPATIAL: {
                alia_box box = frag->box;
                alia_element_box_region(
                    ctx, id, &box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
                break;
            }
            case ALIA_CATEGORY_DRAWING: {
                alia_srgba8 const color = alia_palette_color_resolve(
                    alia_ctx_palette(ctx), effective_style->color);
                cache->engine->vtable->draw_block_range(
                    cache->engine,
                    ctx,
                    z,
                    cache->block,
                    frag->byte_start,
                    frag->byte_end,
                    frag->baseline_origin,
                    color,
                    ALIA_TEXT_DIRECTION_LTR);
                break;
            }
            default:
                break;
        }
    }
}

ALIA_EXTERN_C_END
