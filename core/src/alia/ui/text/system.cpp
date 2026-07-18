#include <alia/abi/ui/text.h>

#include <alia/abi/kernel/substrate.h>
#include <alia/impl/base/stack.hpp>
#include <alia/ui/system/object.h>

namespace {

// a single entry in the active-font scope chain - Entries are allocated on
// `alia_context::stack` and remember the previously active font for popping.
// The resolved font itself lives in the substrate (produced by
// `alia_resolve_font`); the scope only borrows it.
struct font_scope
{
    alia_resolved_font const* parent;
};

} // namespace

extern "C" {

// TYPEFACE REGISTRY

alia_typeface_id
alia_register_typeface(
    alia_ui_system* system, alia_text_engine* engine, void* engine_handle)
{
    ALIA_ASSERT(system);
    // The system must have been initialized with a reserved "invalid" typeface
    // (index 0) before registering any real typefaces.
    ALIA_ASSERT(!system->typefaces.empty());

    alia_typeface_id const id = (alia_typeface_id) system->typefaces.size();
    system->typefaces.push_back(
        alia_resolved_typeface{id, engine, engine_handle});
    return id;
}

alia_resolved_typeface
alia_typeface_resolve(alia_ui_system* system, alia_typeface_id typeface)
{
    ALIA_ASSERT(system);
    ALIA_ASSERT(
        typeface != ALIA_TYPEFACE_ID_INVALID
        && typeface < system->typefaces.size());
    return system->typefaces[typeface];
}

// FONT RESOLUTION

alia_resolved_font const*
alia_resolve_font(alia_context* ctx, alia_font font)
{
    ALIA_ASSERT(ctx && ctx->system);

    alia_substrate_usage_result const result = alia_substrate_use_memory(
        ctx, sizeof(alia_resolved_font), alignof(alia_resolved_font));
    alia_resolved_font* const slot = (alia_resolved_font*) result.ptr;

    // On INIT/DISCOVERY the slot is fresh (don't read it). On NORMAL it has
    // been seen before, so it's safe to compare. Recompute whenever the
    // requested font differs from what's cached.
    bool const fresh = result.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;
    if (fresh || slot->typeface.id != font.typeface || slot->size != font.size)
    {
        alia_resolved_typeface const typeface
            = alia_typeface_resolve(ctx->system, font.typeface);
        ALIA_ASSERT(
            typeface.engine && typeface.engine->vtable
            && typeface.engine->vtable->get_font_metrics);
        slot->typeface = typeface;
        slot->size = font.size;
        typeface.engine->vtable->get_font_metrics(
            typeface.engine,
            typeface.engine_handle,
            font.size,
            &slot->metrics);
    }
    return slot;
}

// FONT SCOPE

void
alia_font_push(alia_context* ctx, alia_resolved_font const* font)
{
    ALIA_ASSERT(ctx && font);
    auto& scope = alia::stack_push<font_scope>(ctx);
    scope.parent = ctx->active_font;
    ctx->active_font = font;
}

void
alia_font_pop(alia_context* ctx)
{
    ALIA_ASSERT(ctx);
    auto& scope = alia::stack_pop<font_scope>(ctx);
    ctx->active_font = scope.parent;
}

} // extern "C"
