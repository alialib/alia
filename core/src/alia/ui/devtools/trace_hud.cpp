#include <alia/abi/ui/devtools/trace_hud.h>

#include <alia/abi/kernel/ids.h>
#include <alia/abi/kernel/substrate.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/drawing/primitives.h>
#include <alia/abi/ui/input/elements.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/api.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/system/tracer.h>
#include <alia/abi/ui/text.h>
#include <alia/impl/events.hpp>
#include <alia/impl/kernel/substrate.hpp>

#include <cstdio>
#include <cstring>

namespace {

alia_palette_color
trace_hud_text_color(enum alia_palette_ramp_level level)
{
    return alia_palette_color_make(
        alia_palette_index_foundation_ramp(
            ALIA_PALETTE_FOUNDATION_RAMP_TEXT, level),
        0xff);
}

void
emit_text(
    alia_context* ctx,
    char const* text,
    alia_resolved_font const* font,
    alia_palette_color color,
    alia_layout_flags_t flags)
{
    alia_text_style const style = {.font = font, .color = color};
    alia_text(ctx, flags, alia_text_literal(text), &style);
}

void
format_pass_line(char* out, size_t out_size, alia_trace_pass const& pass)
{
    int64_t const us = alia_trace_ticks_to_ns(pass.end - pass.start) / 1000;
    if (pass.kind == ALIA_TRACE_PASS_REFRESH)
    {
        std::snprintf(
            out,
            out_size,
            "%-12s #%u%s %6lld us",
            alia_trace_pass_kind_name(pass.kind),
            unsigned(pass.refresh.index),
            pass.refresh.incomplete ? "*" : " ",
            static_cast<long long>(us));
    }
    else if (pass.kind == ALIA_TRACE_PASS_EVENT)
    {
        std::snprintf(
            out,
            out_size,
            "%-12s 0x%-6x %6lld us",
            alia_trace_pass_kind_name(pass.kind),
            unsigned(pass.event.type),
            static_cast<long long>(us));
    }
    else
    {
        std::snprintf(
            out,
            out_size,
            "%-12s         %6lld us",
            alia_trace_pass_kind_name(pass.kind),
            static_cast<long long>(us));
    }
}

void
pass_list(
    alia_context* ctx,
    alia_trace_frame const* frame,
    alia_resolved_font const* font)
{
    if (!frame)
        return;

    // Keyed per-pass lines keep the enclosing expand ALIA_IF's substrate size
    // independent of pass_count (the key table is fixed-size in the parent).
    alia_substrate_key_table* table
        = alia_substrate_use_key_table(ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    alia::key_scope(ctx, table, [&](alia_substrate_key_scope* scope) {
        for (uint16_t i = 0; i < frame->pass_count; ++i)
        {
            alia::keyed_block(ctx, scope, alia_id_view_make_u32(i), [&] {
                char line[96];
                format_pass_line(line, sizeof(line), frame->passes[i]);
                emit_text(
                    ctx,
                    line,
                    font,
                    trace_hud_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE),
                    ALIA_FLUSH | ALIA_ALIGN_LEFT);
            });
        }

        if (frame->dropped_passes != 0)
        {
            alia::keyed_block(
                ctx, scope, alia_id_view_make_u32(0xffffffffu), [&] {
                    char dropped[64];
                    std::snprintf(
                        dropped,
                        sizeof(dropped),
                        "dropped %u passes",
                        unsigned(frame->dropped_passes));
                    emit_text(
                        ctx,
                        dropped,
                        font,
                        trace_hud_text_color(ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
                        ALIA_FLUSH | ALIA_ALIGN_LEFT);
                });
        }
    });
}

alia_trace_frame const*
find_traced_frame(alia_ui_tracer const* tracer, uint64_t index)
{
    uint32_t const limit = tracer->stored_count < ALIA_TRACE_RING_SIZE
                             ? tracer->stored_count
                             : ALIA_TRACE_RING_SIZE;
    for (uint32_t i = 0; i < limit; ++i)
    {
        if (tracer->frames[i].index == index)
            return &tracer->frames[i];
    }
    return nullptr;
}

bool
timing_header(
    alia_context* ctx,
    char const* label,
    alia_srgb8 fill,
    alia_palette_color text_color,
    alia_resolved_font const* font,
    bool* expanded)
{
    alia_element_id const id = alia_element_get_identity(ctx);
    bool toggled = false;

    alia_layout_column_begin(
        ctx, ALIA_FILL_X | ALIA_FLUSH | ALIA_PROVIDE_BOX, 0.f);

    if (!alia::is_refresh_event(*ctx))
    {
        alia_box const box = alia_layout_consume_box(ctx);

        alia_element_box_region(
            ctx, id, &box, ALIA_CURSOR_POINTER, ALIA_HIT_TEST_MOUSE);

        alia_event_category const category = alia::get_event_category(*ctx);
        if (category == ALIA_CATEGORY_INPUT
            && alia_element_detect_click(ctx, id, ALIA_BUTTON_LEFT))
        {
            *expanded = !*expanded;
            toggled = true;
        }

        if (category == ALIA_CATEGORY_DRAWING)
        {
            alia_draw_box(
                ctx,
                ctx->geometry->z_base,
                box,
                {.fill_color = alia_srgba8_from_srgb8(fill),
                 .corner_radius = 0.f,
                 .border_width = 0.f,
                 .border_color = alia_srgba8_from_srgb8(fill)});
            ++ctx->geometry->z_base;
        }
    }

    alia_edge_offsets const padding
        = alia_edge_offsets_make_trbl(4.f, 8.f, 4.f, 8.f);
    alia_layout_edge_offsets_begin(ctx, padding, ALIA_FLUSH);
    emit_text(ctx, label, font, text_color, ALIA_FLUSH | ALIA_ALIGN_LEFT);
    alia_layout_edge_offsets_end(ctx);

    alia_layout_column_end(ctx);
    return toggled;
}

void
emit_expanded_body(
    alia_context* ctx,
    alia_trace_frame const* frame,
    alia_resolved_font const* font,
    char const* optional_heading)
{
    alia_edge_offsets const padding
        = alia_edge_offsets_make_trbl(2.f, 8.f, 4.f, 8.f);
    alia_layout_edge_offsets_begin(ctx, padding, ALIA_FLUSH);
    // edge_offsets only measures/assigns its first child; wrap the body
    // lines in a column so every pass text gets a placement record.
    alia_layout_column_begin(ctx, ALIA_FLUSH | ALIA_ALIGN_LEFT, 0.f);
    if (optional_heading)
    {
        emit_text(
            ctx,
            optional_heading,
            font,
            trace_hud_text_color(ALIA_PALETTE_RAMP_LEVEL_WEAKER_1),
            ALIA_FLUSH | ALIA_ALIGN_LEFT);
    }
    pass_list(ctx, frame, font);
    alia_layout_column_end(ctx);
    alia_layout_edge_offsets_end(ctx);
}

bool
summary_has_spike(alia_trace_summary const* summary, uint64_t frame_index)
{
    for (uint32_t i = 0; i < summary->spike_count; ++i)
    {
        if (summary->spikes[i] && summary->spikes[i]->index == frame_index)
            return true;
    }
    return false;
}

alia_trace_frame const*
summary_spike_frame(alia_trace_summary const* summary, uint64_t frame_index)
{
    for (uint32_t i = 0; i < summary->spike_count; ++i)
    {
        if (summary->spikes[i] && summary->spikes[i]->index == frame_index)
            return summary->spikes[i];
    }
    return nullptr;
}

void
refresh_hud_snapshots(
    alia_trace_hud_view_state* state,
    alia_ui_tracer const* tracer,
    alia_trace_summary const* summary)
{
    alia_trace_hud_spike_row previous[ALIA_TRACE_HUD_MAX_ROWS];
    uint32_t const previous_count = state->spike_count;
    if (previous_count != 0)
    {
        std::memcpy(
            previous, state->spikes, sizeof(previous[0]) * previous_count);
    }

    // Preserve existing row order. Rebuilding from live top-K every refresh
    // reshuffles headers (flicker) and can change element identities between
    // mouse press and release (clicks stop working).
    uint32_t out = 0;
    for (uint32_t i = 0; i < previous_count; ++i)
    {
        uint64_t const index = previous[i].frame.index;
        bool const still_spike = summary_has_spike(summary, index);
        if (!still_spike && !previous[i].expanded)
            continue;
        if (out >= ALIA_TRACE_HUD_MAX_ROWS)
            break;

        alia_trace_hud_spike_row& row = state->spikes[out++];
        if (alia_trace_frame const* src = summary_spike_frame(summary, index))
            row.frame = *src;
        else if (
            alia_trace_frame const* live = find_traced_frame(tracer, index))
            row.frame = *live;
        else
            row.frame = previous[i].frame;
        row.expanded = previous[i].expanded;
        row.pinned = previous[i].expanded && !still_spike;
    }

    for (uint32_t i = 0; i < summary->spike_count; ++i)
    {
        if (out >= ALIA_TRACE_HUD_MAX_ROWS)
            break;
        alia_trace_frame const* src = summary->spikes[i];
        if (!src)
            continue;
        bool already = false;
        for (uint32_t j = 0; j < out; ++j)
        {
            if (state->spikes[j].frame.index == src->index)
            {
                already = true;
                break;
            }
        }
        if (already)
            continue;
        alia_trace_hud_spike_row& row = state->spikes[out++];
        row.frame = *src;
        row.expanded = false;
        row.pinned = false;
    }

    state->spike_count = out;
    state->spike_overflow = summary->spike_overflow;

    if (summary->representative)
    {
        state->representative = *summary->representative;
        state->has_representative = true;
    }
    else if (!state->avg_expanded)
    {
        state->has_representative = false;
    }
}

void
draw_representative_section(
    alia_context* ctx,
    alia_ui_tracer const* tracer,
    alia_trace_hud_view_state* state,
    bool avg_expanded_this_pass,
    bool has_representative_this_pass,
    int64_t average_us,
    alia_resolved_font const* font,
    alia_srgb8 avg_fill,
    alia_palette_color avg_text,
    bool* needs_abort)
{
    char label[64];
    if (tracer->stored_count == 0)
        std::snprintf(label, sizeof(label), "      - us");
    else
        std::snprintf(
            label,
            sizeof(label),
            "%7lld us",
            static_cast<long long>(average_us));

    // Pass-start snapshots only - click mutations must not change which
    // bodies run later in this same traversal.
    bool const show_body
        = avg_expanded_this_pass && has_representative_this_pass;
    bool expanded = show_body;
    if (timing_header(ctx, label, avg_fill, avg_text, font, &expanded))
    {
        state->avg_expanded = expanded;
        *needs_abort = true;
    }

    ALIA_IF (show_body)
    {
        char rep_label[48];
        std::snprintf(
            rep_label,
            sizeof(rep_label),
            "rep #%llu",
            static_cast<unsigned long long>(state->representative.index));
        emit_expanded_body(ctx, &state->representative, font, rep_label);
    }
    ALIA_END
}

void
draw_spike_row(
    alia_context* ctx,
    alia_trace_hud_view_state* state,
    uint32_t slot,
    bool expanded_this_pass,
    alia_resolved_font const* font,
    alia_srgb8 spike_fill,
    alia_palette_color spike_text,
    bool* needs_abort)
{
    alia_trace_hud_spike_row& row = state->spikes[slot];
    uint64_t const frame_index = row.frame.index;

    char label[64];
    std::snprintf(
        label,
        sizeof(label),
        "%7lld us  #%llu%s",
        static_cast<long long>(alia_trace_frame_wall_us(&row.frame)),
        static_cast<unsigned long long>(frame_index),
        row.pinned ? " *" : "");

    bool const show_body = expanded_this_pass;
    bool expanded = show_body;
    if (timing_header(ctx, label, spike_fill, spike_text, font, &expanded))
    {
        row.expanded = expanded;
        if (!expanded && row.pinned)
        {
            // Drop sticky leftovers on collapse at the next refresh rebuild.
            row.pinned = false;
        }
        *needs_abort = true;
    }

    ALIA_IF (show_body)
    {
        emit_expanded_body(ctx, &row.frame, font, nullptr);
    }
    ALIA_END
}

} // namespace

extern "C" {

void
alia_trace_hud_view_state_init(alia_trace_hud_view_state* state)
{
    ALIA_ASSERT(state);
    std::memset(state, 0, sizeof(*state));
    alia_trace_summary_state_init(&state->summary_state);
    state->has_representative = false;
    state->avg_expanded = false;
}

bool
alia_ui_trace_hud(
    alia_context* ctx,
    alia_ui_tracer const* tracer,
    alia_trace_hud_view_state* state,
    alia_trace_hud_style const* style,
    alia_trace_summary_config const* config)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(tracer);
    ALIA_ASSERT(state);
    ALIA_ASSERT(style);
    ALIA_ASSERT(style->font);

    alia_trace_summary const summary
        = alia_trace_summarize(tracer, config, &state->summary_state);

    if (alia::is_refresh_event(*ctx))
        refresh_hud_snapshots(state, tracer, &summary);

    // Freeze expand / presence decisions for this entire traversal.
    bool avg_expanded_this_pass = state->avg_expanded;
    bool has_representative_this_pass = state->has_representative;
    bool expanded_this_pass[ALIA_TRACE_HUD_MAX_ROWS];
    uint32_t const spike_count_this_pass = state->spike_count;
    for (uint32_t i = 0; i < spike_count_this_pass; ++i)
        expanded_this_pass[i] = state->spikes[i].expanded;

    alia_palette const* palette = alia_ctx_palette(ctx);
    alia_srgb8 const avg_fill = palette->foundation.background.stronger_2;
    alia_srgb8 const spike_fill = palette->danger.solid;
    alia_palette_color const avg_text
        = trace_hud_text_color(ALIA_PALETTE_RAMP_LEVEL_BASE);
    alia_palette_color const spike_text = alia_palette_color_make(
        alia_palette_index_swatch(
            ALIA_PALETTE_SWATCH_DANGER, ALIA_PALETTE_SWATCH_PART_ON_SOLID),
        0xff);

    bool needs_abort = false;

    alia_layout_column_begin(
        ctx, ALIA_FLUSH | ALIA_ALIGN_RIGHT | ALIA_ALIGN_TOP, 0.f);

    draw_representative_section(
        ctx,
        tracer,
        state,
        avg_expanded_this_pass,
        has_representative_this_pass,
        summary.average_us,
        style->font,
        avg_fill,
        avg_text,
        &needs_abort);

    alia_substrate_key_table* spike_table
        = alia_substrate_use_key_table(ctx, ALIA_SUBSTRATE_KEY_TABLE_NORMAL);
    alia::key_scope(ctx, spike_table, [&](alia_substrate_key_scope* scope) {
        for (uint32_t slot = 0; slot < spike_count_this_pass; ++slot)
        {
            uint64_t const frame_index = state->spikes[slot].frame.index;
            alia::keyed_block(
                ctx, scope, alia_id_view_make_u64(frame_index), [&] {
                    draw_spike_row(
                        ctx,
                        state,
                        slot,
                        expanded_this_pass[slot],
                        style->font,
                        spike_fill,
                        spike_text,
                        &needs_abort);
                });
        }
    });

    ALIA_IF (state->spike_overflow != 0)
    {
        char more[32];
        std::snprintf(
            more, sizeof(more), "+%u more", unsigned(state->spike_overflow));
        emit_text(
            ctx,
            more,
            style->font,
            trace_hud_text_color(ALIA_PALETTE_RAMP_LEVEL_WEAKER_2),
            ALIA_FLUSH | ALIA_ALIGN_RIGHT);
    }
    ALIA_END

    alia_layout_column_end(ctx);

    return needs_abort;
}

} // extern "C"
