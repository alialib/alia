#include <alia/abi/kernel/substrate.h>

#include <alia/abi/context.h>
#include <alia/abi/kernel/ids.h>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/kernel/substrate.h>

#include <algorithm>
#include <unordered_map>
#include <vector>

struct alia_substrate_key_map
{
    struct view_key
    {
        alia_id_view view;
    };

    struct view_key_hash
    {
        size_t
        operator()(view_key const& key) const
        {
            return alia_id_view_hash(key.view);
        }
    };

    struct view_key_eq
    {
        bool
        operator()(view_key const& a, view_key const& b) const
        {
            return alia_id_view_equals(a.view, b.view);
        }
    };

    std::unordered_map<
        view_key,
        alia_substrate_key_entry*,
        view_key_hash,
        view_key_eq>
        entries;
};

// Visibility tiers in this file:
// 1. C ABI — alia_substrate_* inside ALIA_EXTERN_C_BEGIN…END
// 2. C++ cross-TU — namespace alia functions declared in substrate.h
// 3. File-local — anonymous namespace helpers

namespace {

void
invoke_cleanup_records(
    alia_substrate_system* system,
    alia_substrate_block* block,
    alia_substrate_cleanup_mode mode)
{
    for (alia_substrate_cleanup_record* d = block->cleanup_records;
         d != nullptr;
         d = d->next)
    {
        d->cleanup(system, d->ptr, mode);
    }
}

void
block_release(alia_substrate_system* system, alia_substrate_block* block)
{
    system->allocator.free(
        system->allocator.user_data,
        block,
        block->spec.size,
        block->spec.align);
    ++system->current_generation;
    ALIA_ASSERT(system->current_generation != 0);
}

void
block_destroy(alia_substrate_system* system, alia_substrate_block* block)
{
    invoke_cleanup_records(system, block, ALIA_SUBSTRATE_DESTROY);
    block_release(system, block);
}

void
anchor_cleanup(
    alia_substrate_system* system, void* ptr, alia_substrate_cleanup_mode mode)
{
    alia_substrate_anchor* anchor = static_cast<alia_substrate_anchor*>(ptr);
    invoke_cleanup_records(system, anchor->block, mode);
    if (mode == ALIA_SUBSTRATE_DESTROY)
        block_release(system, anchor->block);
}

alia_substrate_block*
init_block(alia_context* ctx, void* ptr, alia_struct_spec spec)
{
    return new (ptr) alia_substrate_block{
        .cleanup_records = nullptr,
        .generation = ctx->substrate->system->current_generation,
        .spec = spec};
}

void
add_cleanup_record(
    alia_context* ctx,
    void* storage,
    void (*cleanup)(
        alia_substrate_system*, void*, alia_substrate_cleanup_mode mode),
    void* object)
{
    auto& traversal = *ctx->substrate;
    alia_substrate_cleanup_record* record
        = new (storage) alia_substrate_cleanup_record{
            traversal.block.block->cleanup_records, cleanup, object};
    traversal.block.block->cleanup_records = record;
}

alia_id_pair*
scratch_alloc_pair(alia_bump_allocator* scratch)
{
    return static_cast<alia_id_pair*>(alia_arena_ptr(
        scratch,
        alia_arena_alloc_aligned(
            scratch, sizeof(alia_id_pair), alignof(alia_id_pair))));
}

alia_id_view
fold_id_segments(
    alia_bump_allocator* scratch, alia_id_view const* segments, size_t count)
{
    ALIA_ASSERT(count > 0);
    alia_id_view acc = segments[count - 1];
    for (size_t i = count - 1; i > 0; --i)
    {
        acc = alia_id_view_make_pair(
            scratch_alloc_pair(scratch), acc, segments[i - 1]);
    }
    return acc;
}

size_t
count_path_segments(alia_substrate_block_traversal_state const* block)
{
    size_t count = 1;
    alia_substrate_block_traversal_state const* state = block;
    while (state->parent != nullptr && state->parent->block != nullptr)
    {
        ++count;
        state = state->parent;
    }
    return count;
}

alia_id_view*
push_path_segments_for_object(
    alia_context* ctx,
    void* object,
    alia_substrate_block_traversal_state const* block,
    size_t* out_count)
{
    alia_bump_allocator* scratch = &ctx->substrate->scratch;
    size_t const count = count_path_segments(block);
    alia_id_view* segments = static_cast<alia_id_view*>(alia_arena_ptr(
        scratch,
        alia_arena_alloc_aligned(
            scratch, count * sizeof(alia_id_view), alignof(alia_id_view))));

    size_t index = 0;
    segments[index++] = alia_id_view_make_u64(
        static_cast<uint64_t>(
            static_cast<std::uint8_t*>(object)
            - static_cast<std::uint8_t*>(static_cast<void*>(block->block))));

    alia_substrate_block_traversal_state const* state = block;
    while (state->parent != nullptr && state->parent->block != nullptr)
    {
        segments[index++] = alia_id_view_make_u64(
            static_cast<uint64_t>(state->offset_in_parent));
        state = state->parent;
    }

    ALIA_ASSERT(index == count);
    *out_count = count;
    return segments;
}

bool
entry_is_collectible(
    alia_substrate_key_table const* table,
    alia_substrate_key_entry const* entry)
{
    return entry->last_seen != table->last_seen
        && (!table->requires_explicit_delete
            || (entry->flags & ALIA_SUBSTRATE_KEY_EXPLICITLY_DELETED) != 0);
}

void
free_key_entry(alia_substrate_system* system, alia_substrate_key_entry* entry)
{
    alia::substrate_reset_anchor(system, &entry->anchor);
    if (entry->key_storage)
    {
        alia_captured_id_release(entry->key_storage);
        alia_struct_spec spec = alia_captured_id_spec(entry->key.view);
        system->allocator.free(
            system->allocator.user_data,
            entry->key_storage,
            spec.size,
            spec.align);
        entry->key_storage = nullptr;
    }
    system->allocator.free(
        system->allocator.user_data,
        entry,
        sizeof(alia_substrate_key_entry),
        alignof(alia_substrate_key_entry));
}

void
clear_key_map_cache(alia_substrate_system* system, alia_substrate_key_map* map)
{
    ALIA_ASSERT(system && map);
    for (auto& [key, entry] : map->entries)
        alia_substrate_deactivate_anchor(system, &entry->anchor);
}

void
destroy_key_map(alia_substrate_system* system, alia_substrate_key_map* map)
{
    if (!map)
        return;
    for (auto& i : map->entries)
        free_key_entry(system, i.second);
    map->entries.clear();
    system->allocator.free(
        system->allocator.user_data,
        map,
        sizeof(alia_substrate_key_map),
        alignof(alia_substrate_key_map));
}

void
unregister_key_table(
    alia_substrate_system* system, alia_substrate_key_table* table)
{
    alia_substrate_key_table** link = &system->key_table_registry;
    while (*link)
    {
        if (*link == table)
        {
            *link = table->registry_next;
            table->registry_next = nullptr;
            return;
        }
        link = &(*link)->registry_next;
    }
}

void
register_key_table(
    alia_substrate_system* system, alia_substrate_key_table* table)
{
    table->registry_next = system->key_table_registry;
    system->key_table_registry = table;
}

void
key_table_cleanup(
    alia_substrate_system* system, void* ptr, alia_substrate_cleanup_mode mode)
{
    auto* table = static_cast<alia_substrate_key_table*>(ptr);
    if (mode == ALIA_SUBSTRATE_CLEAR_CACHE)
    {
        if (table->map)
            clear_key_map_cache(system, table->map);
        return;
    }

    destroy_key_map(system, table->map);
    table->map = nullptr;
    unregister_key_table(system, table);
    table->first = nullptr;
}

alia_substrate_key_map*
create_key_map(alia_substrate_system* system)
{
    void* mem = system->allocator.alloc(
        system->allocator.user_data,
        sizeof(alia_substrate_key_map),
        alignof(alia_substrate_key_map));
    return new (mem) alia_substrate_key_map();
}

alia_substrate_key_entry*
find_entry_by_view(alia_substrate_key_map* map, alia_id_view view)
{
    alia_substrate_key_map::view_key key{view};
    auto i = map->entries.find(key);
    return i == map->entries.end() ? nullptr : i->second;
}

alia_substrate_key_entry*
create_key_entry(
    alia_substrate_system* system,
    alia_substrate_key_table* table,
    alia_id_view key)
{
    alia_struct_spec key_spec = alia_captured_id_spec(key);
    void* key_storage = system->allocator.alloc(
        system->allocator.user_data, key_spec.size, key_spec.align);
    alia_captured_id_capture_into(key, key_storage, key_spec.size);

    void* entry_mem = system->allocator.alloc(
        system->allocator.user_data,
        sizeof(alia_substrate_key_entry),
        alignof(alia_substrate_key_entry));
    auto* entry = new (entry_mem) alia_substrate_key_entry{
        .anchor = {.block = nullptr},
        .key = *static_cast<alia_captured_id*>(key_storage),
        .key_storage = key_storage,
        .last_seen = 0,
        .flags = 0,
        .next = nullptr};

    alia_substrate_key_map::view_key map_key{entry->key.view};
    table->map->entries.emplace(map_key, entry);
    return entry;
}

void
append_to_prediction_list(
    alia_substrate_key_scope* scope, alia_substrate_key_entry* entry)
{
    alia_substrate_key_entry* const old_next = entry->next;
    *scope->prediction_tail = entry;
    entry->next = nullptr;
    scope->prediction_tail = &entry->next;
    scope->predicted = old_next;
}

alia_substrate_key_entry*
resolve_key_entry(
    alia_context* ctx, alia_substrate_key_scope* scope, alia_id_view key)
{
    alia_substrate_key_entry* entry = nullptr;
    if (scope->predicted
        && alia_captured_id_matches_view(&scope->predicted->key, key))
    {
        entry = scope->predicted;
        scope->predicted = entry->next;
        if (ctx->substrate->allow_prediction_updates)
            scope->prediction_tail = &entry->next;
    }
    else
    {
        entry = find_entry_by_view(scope->table->map, key);
        if (!entry)
        {
            entry
                = create_key_entry(ctx->substrate->system, scope->table, key);
        }
        if (ctx->substrate->allow_prediction_updates)
            append_to_prediction_list(scope, entry);
    }

    entry->last_seen = ctx->substrate->current_frame;
    return entry;
}

void
sweep_key_table(alia_substrate_key_table* table)
{
    if (!table || !table->map)
        return;

    alia_substrate_system* system = table->system;
    std::vector<alia_substrate_key_entry*> to_remove;
    to_remove.reserve(table->map->entries.size());
    for (auto& i : table->map->entries)
    {
        if (entry_is_collectible(table, i.second))
            to_remove.push_back(i.second);
    }

    for (alia_substrate_key_entry* entry : to_remove)
    {
        alia_substrate_key_map::view_key map_key{entry->key.view};
        table->map->entries.erase(map_key);
        free_key_entry(system, entry);
    }
}

} // namespace

namespace alia {

void
substrate_system_init(
    alia_substrate_system& system, alia_general_allocator allocator)
{
    memset(&system, 0, sizeof(system));
    system.allocator = allocator;
}

void
substrate_system_destroy(alia_substrate_system& system)
{
    block_destroy(&system, system.root_anchor.block);
    system.root_anchor.block = nullptr;
}

void
substrate_system_reset(alia_substrate_system& system)
{
    if (system.root_anchor.block)
    {
        block_destroy(&system, system.root_anchor.block);
        system.root_anchor.block = nullptr;
    }
}

void
substrate_traversal_init(
    alia_substrate_traversal& traversal,
    alia_substrate_system& system,
    alia_bump_allocator* scratch,
    uint32_t current_frame,
    bool allow_prediction_updates)
{
    memset(&traversal, 0, sizeof(traversal));
    traversal.system = &system;
    traversal.scratch = *scratch;
    traversal.current_frame = current_frame;
    traversal.allow_prediction_updates = allow_prediction_updates;
}

void
substrate_reset_anchor(
    alia_substrate_system* system, alia_substrate_anchor* anchor)
{
    if (anchor->block)
    {
        block_destroy(system, anchor->block);
        anchor->block = nullptr;
    }
}

} // namespace alia

ALIA_EXTERN_C_BEGIN

alia_substrate_system*
alia_ctx_substrate_system(alia_context* ctx)
{
    ALIA_ASSERT(ctx && ctx->substrate && ctx->substrate->system);
    return ctx->substrate->system;
}

alia_substrate_usage_result
alia_substrate_use_memory(alia_context* ctx, size_t size, size_t alignment)
{
    auto& traversal = *ctx->substrate;
    switch (traversal.block.mode)
    {
        case ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY: {
            traversal.block.spec.size
                = alia::align_offset(traversal.block.spec.size, alignment)
                + size;
            traversal.block.spec.align
                = std::max(alignment, traversal.block.spec.align);
            size_t arena_alignment
                = (std::max) (alignment, size_t(ALIA_MIN_ALIGN));
            size_t arena_size = alia::align_offset(size, arena_alignment);
            return {
                alia_arena_ptr(
                    &traversal.scratch,
                    alia_arena_alloc_aligned(
                        &traversal.scratch, arena_size, arena_alignment)),
                traversal.block.block->generation,
                ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY};
        }
        case ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL:
        case ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT:
        default:
            size_t aligned_offset = alia::align_offset(
                traversal.block.current_offset, alignment);
            traversal.block.current_offset = aligned_offset + size;
            return {
                reinterpret_cast<std::uint8_t*>(traversal.block.block)
                    + aligned_offset,
                traversal.block.block->generation,
                traversal.block.mode};
    }
}

alia_substrate_usage_result
alia_substrate_use_object(
    alia_context* ctx,
    size_t size,
    size_t alignment,
    void (*cleanup)(
        alia_substrate_system*, void*, alia_substrate_cleanup_mode mode))
{
    // 'Use' the memory for the object.
    alia_substrate_usage_result mr
        = alia_substrate_use_memory(ctx, size, alignment);
    // 'Use' the memory for the destructor record.
    alia_substrate_usage_result dr = alia_substrate_use_memory(
        ctx,
        sizeof(alia_substrate_cleanup_record),
        alignof(alia_substrate_cleanup_record));
    // If the destructor record is new, initialize it and add it to the block's
    // destructor list.
    if (dr.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
        add_cleanup_record(ctx, dr.ptr, cleanup, mr.ptr);
    return mr;
}

alia_substrate_anchor*
alia_substrate_use_anchor(alia_context* ctx)
{
    // TODO: Combine these two allocations/conditions into one.
    alia_substrate_usage_result mr = alia_substrate_use_memory(
        ctx, sizeof(alia_substrate_anchor), alignof(alia_substrate_anchor));
    if (mr.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
    {
        new (mr.ptr) alia_substrate_anchor{.block = nullptr};
    }
    alia_substrate_usage_result dr = alia_substrate_use_memory(
        ctx,
        sizeof(alia_substrate_cleanup_record),
        alignof(alia_substrate_cleanup_record));
    if (dr.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT)
    {
        add_cleanup_record(ctx, dr.ptr, anchor_cleanup, mr.ptr);
    }
    return static_cast<alia_substrate_anchor*>(mr.ptr);
}

void
alia_substrate_reset_anchor(
    alia_substrate_system* system, alia_substrate_anchor* anchor)
{
    alia::substrate_reset_anchor(system, anchor);
}

void
alia_substrate_deactivate_anchor(
    alia_substrate_system* system, alia_substrate_anchor* anchor)
{
    if (anchor->block)
    {
        invoke_cleanup_records(
            system, anchor->block, ALIA_SUBSTRATE_CLEAR_CACHE);
    }
}

// Begin the scope of a child substrate block.
// `spec` is the memoized memory layout specification of the block.
// If `alia_substrate_block_needs_discovery` returns true on the spec, the
// traversal will enter discovery mode for the block.
void
alia_substrate_begin_block(
    alia_context* ctx, alia_substrate_anchor* anchor, alia_struct_spec* spec)
{
    auto& parent_scope
        = alia::stack_push<alia_substrate_block_traversal_state>(ctx);

    auto& traversal = *ctx->substrate;
    parent_scope = traversal.block;
    traversal.block.parent = &parent_scope;
    traversal.block.offset_in_parent = parent_scope.current_offset;
    traversal.block.current_offset = sizeof(alia_substrate_block);

    if (alia_substrate_block_needs_discovery(spec))
    {
        ALIA_ASSERT(alia::get_event_type(*ctx) == ALIA_EVENT_REFRESH);
        alia::as_refresh_event(*ctx).incomplete = true;
        traversal.block.block = init_block(
            ctx,
            alia_arena_ptr(
                &traversal.scratch,
                alia_arena_alloc_aligned(
                    &traversal.scratch,
                    sizeof(alia_substrate_block),
                    alignof(alia_substrate_block))),
            {0, 0});
        traversal.block.spec
            = {.size = sizeof(alia_substrate_block),
               .align = alignof(alia_substrate_block)};
        traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY;
    }
    else
    {
        traversal.block.spec = *spec;
        if (anchor->block == nullptr)
        {
            anchor->block = init_block(
                ctx,
                traversal.system->allocator.alloc(
                    traversal.system->allocator.user_data,
                    spec->size,
                    spec->align),
                *spec);
            traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_INIT;
        }
        else
        {
            traversal.block.mode = ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL;
        }
        traversal.block.block = anchor->block;
    }
}

// End the scope of the current substrate block.
// Returns the layout specification of the block. - If the block was in
// discovery mode, this should be used to update the memoized spec value.
alia_struct_spec
alia_substrate_end_block(alia_context* ctx)
{
    if (ctx->substrate->block.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY)
    {
        invoke_cleanup_records(
            ctx->substrate->system,
            ctx->substrate->block.block,
            ALIA_SUBSTRATE_DESTROY);
    }
    auto& parent_scope
        = alia::stack_pop<alia_substrate_block_traversal_state>(ctx);
    auto& traversal = *ctx->substrate;
    alia_struct_spec const completed_block_spec = traversal.block.spec;
    traversal.block = parent_scope;
    return completed_block_spec;
}

alia_id_view
alia_substrate_path_for_object(alia_context* ctx, void* object)
{
    ALIA_ASSERT(ctx && ctx->substrate && object);

    alia_substrate_block_traversal_state const& block = ctx->substrate->block;
    if (block.mode == ALIA_SUBSTRATE_BLOCK_TRAVERSAL_DISCOVERY)
        return alia_id_view_null();

    ALIA_ASSERT(block.block);
    auto* block_base
        = static_cast<std::uint8_t*>(static_cast<void*>(block.block));
    auto* block_end = block_base + block.spec.size;
    auto* object_ptr = static_cast<std::uint8_t*>(object);
    ALIA_ASSERT(object_ptr >= block_base && object_ptr < block_end);

    size_t count = 0;
    alia_id_view* segments
        = push_path_segments_for_object(ctx, object, &block, &count);
    return fold_id_segments(&ctx->substrate->scratch, segments, count);
}

alia_substrate_key_table*
alia_substrate_use_key_table(
    alia_context* ctx, alia_substrate_key_table_flags flags)
{
    alia_substrate_usage_result mr = alia_substrate_use_object(
        ctx,
        sizeof(alia_substrate_key_table),
        alignof(alia_substrate_key_table),
        key_table_cleanup);
    auto* table = static_cast<alia_substrate_key_table*>(mr.ptr);
    if (mr.mode != ALIA_SUBSTRATE_BLOCK_TRAVERSAL_NORMAL)
    {
        alia_substrate_system* system = ctx->substrate->system;
        table->requires_explicit_delete
            = flags == ALIA_SUBSTRATE_KEY_TABLE_REQUIRES_EXPLICIT_DELETE;
        table->first = nullptr;
        table->map = create_key_map(system);
        table->system = system;
        table->registry_next = nullptr;
        register_key_table(system, table);
    }
    return table;
}

alia_substrate_key_scope*
alia_substrate_begin_key_scope(
    alia_context* ctx, alia_substrate_key_table* table)
{
    ALIA_ASSERT(ctx && table);
    auto& scope = alia::stack_push<alia_substrate_key_scope>(ctx);
    scope.table = table;
    scope.predicted = table->first;
    scope.prediction_tail = &table->first;
    table->last_seen = ctx->substrate->current_frame;
    return &scope;
}

void
alia_substrate_end_key_scope(
    alia_context* ctx, alia_substrate_key_scope* scope)
{
    ALIA_ASSERT(ctx && scope && scope->table);

    if (ctx->substrate->allow_prediction_updates && scope->prediction_tail)
        *scope->prediction_tail = nullptr;

    alia::stack_pop<alia_substrate_key_scope>(ctx);
}

void
alia_substrate_begin_keyed_block(
    alia_context* ctx,
    alia_substrate_key_scope* scope,
    alia_id_view key,
    alia_struct_spec* spec)
{
    ALIA_ASSERT(ctx && scope && scope->table);
    alia_substrate_key_entry* entry = resolve_key_entry(ctx, scope, key);
    alia_substrate_begin_block(ctx, &entry->anchor, spec);
}

void
alia_substrate_end_keyed_block(alia_context* ctx)
{
    alia_substrate_end_block(ctx);
}

void
alia_substrate_sweep_table_keys(
    alia_context* ctx, alia_substrate_key_table* table)
{
    ALIA_ASSERT(ctx && ctx->substrate && table);
    sweep_key_table(table);
}

void
alia_substrate_sweep_system_keys(alia_substrate_system* system)
{
    ALIA_ASSERT(system);
    for (alia_substrate_key_table* table = system->key_table_registry; table;
         table = table->registry_next)
    {
        sweep_key_table(table);
    }
}

void
alia_substrate_delete_key(alia_substrate_key_table* table, alia_id_view key)
{
    ALIA_ASSERT(table && table->map);
    alia_substrate_key_entry* entry = find_entry_by_view(table->map, key);
    if (!entry)
        return;
    entry->flags |= ALIA_SUBSTRATE_KEY_EXPLICITLY_DELETED;
}

ALIA_EXTERN_C_END
