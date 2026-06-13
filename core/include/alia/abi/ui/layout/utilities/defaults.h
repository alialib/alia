#ifndef ALIA_ABI_UI_LAYOUT_UTILITIES_DEFAULTS_H
#define ALIA_ABI_UI_LAYOUT_UTILITIES_DEFAULTS_H

#include <alia/abi/ui/layout/protocol.h>

ALIA_EXTERN_C_BEGIN

alia_flow_emission_counts
alia_default_count_flow_emissions(
    alia_measurement_context* ctx, alia_layout_node* node);

void
alia_default_emit_flow_fragments(
    alia_measurement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_emitter* emitter);

void
alia_default_read_fragment_placements(
    alia_placement_context* ctx,
    alia_layout_node* node,
    alia_flow_fragment_reader* reader);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_UTILITIES_DEFAULTS_H
