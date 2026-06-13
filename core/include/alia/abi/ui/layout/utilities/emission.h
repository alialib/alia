#ifndef ALIA_ABI_UI_LAYOUT_UTILITIES_EMISSION_H
#define ALIA_ABI_UI_LAYOUT_UTILITIES_EMISSION_H

#include <alia/abi/context.h>
#include <alia/abi/prelude.h>
#include <alia/abi/ui/layout/flags.h>
#include <alia/abi/ui/layout/protocol.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_layout_emission
{
    alia_bump_allocator arena;
    alia_layout_node** next_ptr;
} alia_layout_emission;

struct alia_layout_context
{
    alia_layout_emission emission;
    alia_bump_allocator placement;
};

static inline alia_bump_allocator*
alia_layout_placement_arena(alia_context* ctx)
{
    // TODO: ALIA_ASSERT(!alia_is_refresh_pass(ctx));
    return &ctx->layout->placement;
}

struct alia_layout_container
{
    alia_layout_node base;
    alia_layout_flags_t flags;
    alia_layout_node* first_child;
};

void
alia_layout_container_activate(
    alia_context* ctx, alia_layout_container* container);

void
alia_layout_container_deactivate(
    alia_context* ctx, alia_layout_container* container);

void
alia_layout_container_simple_begin(
    alia_context* ctx,
    alia_layout_node_vtable* vtable,
    alia_layout_flags_t flags);

void
alia_layout_container_simple_end(alia_context* ctx);

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_UTILITIES_EMISSION_H
