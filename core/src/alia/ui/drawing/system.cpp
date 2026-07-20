#include <alia/abi/prelude.h>
#include <alia/abi/ui/drawing/commands.h>
#include <alia/abi/ui/drawing/system.h>
#include <alia/abi/ui/drawing/targets.h>
#include <alia/abi/ui/system/renderer.h>

#include <alia/abi/ui/events.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/prelude.hpp>
#include <alia/ui/drawing/bucket_key.h>
#include <alia/ui/drawing/system.h>
#include <alia/ui/system/object.h>

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

// Kahn topological order: children (composited sources) before parents.
std::vector<alia_draw_target_id>
order_draw_targets(
    std::unordered_set<alia_draw_target_id> const& targets,
    std::vector<std::pair<alia_draw_target_id, alia_draw_target_id>> const&
        edges)
{
    // edge: child -> parent (child must be flushed first)
    std::unordered_map<alia_draw_target_id, int> indegree;
    std::unordered_map<alia_draw_target_id, std::vector<alia_draw_target_id>>
        outs;
    for (auto id : targets)
        indegree[id] = 0;

    for (auto const& e : edges)
    {
        alia_draw_target_id const child = e.first;
        alia_draw_target_id const parent = e.second;
        if (!targets.count(child) || !targets.count(parent))
            continue;
        outs[child].push_back(parent);
        ++indegree[parent];
    }

    std::vector<alia_draw_target_id> ready;
    ready.reserve(targets.size());
    for (auto id : targets)
    {
        if (indegree[id] == 0)
            ready.push_back(id);
    }
    // Prefer painting non-primary before primary when both are ready.
    std::sort(ready.begin(), ready.end(), [](auto a, auto b) {
        if ((a == ALIA_DRAW_TARGET_PRIMARY) != (b == ALIA_DRAW_TARGET_PRIMARY))
            return a != ALIA_DRAW_TARGET_PRIMARY;
        return a < b;
    });

    std::vector<alia_draw_target_id> ordered;
    ordered.reserve(targets.size());
    size_t qi = 0;
    while (qi < ready.size())
    {
        alia_draw_target_id const id = ready[qi++];
        ordered.push_back(id);
        for (auto parent : outs[id])
        {
            if (--indegree[parent] == 0)
                ready.push_back(parent);
        }
    }

    // Cycle or missing nodes: append anything left (stable enough for debug).
    if (ordered.size() != targets.size())
    {
        for (auto id : targets)
        {
            if (std::find(ordered.begin(), ordered.end(), id) == ordered.end())
                ordered.push_back(id);
        }
    }
    return ordered;
}

} // namespace

extern "C" {

alia_draw_material_id
alia_material_alloc_ids(alia_ui_system* system, uint32_t count)
{
    if (system->draw.next_material_id < ALIA_BUILTIN_MATERIAL_COUNT)
        system->draw.next_material_id = ALIA_BUILTIN_MATERIAL_COUNT;
    auto const start_id = system->draw.next_material_id;
    system->draw.next_material_id += count;
    return start_id;
}

void
alia_material_register(
    alia_ui_system* system,
    alia_draw_material_id id,
    alia_material_vtable vtable,
    void* user)
{
    if (id >= system->draw.materials.size())
        system->draw.materials.resize(size_t(id) + 1);
    system->draw.materials[id] = {.vtable = vtable, .user = user};
}

void
alia_ui_execute_draw_pass(alia_ui_system* system)
{
    ALIA_ASSERT(system);

    alia_draw_bucket_table bucket_table = {
        .buckets = {},
        .keys = {},
    };

    alia_draw_context draw_context = {
        .buckets = &bucket_table,
        .arena = {},
        .target_id = ALIA_DRAW_TARGET_PRIMARY,
    };
    alia_bump_allocator_init(&draw_context.arena, &system->draw.command_arena);

    auto draw_event = alia_make_draw_event({.context = &draw_context});
    alia::dispatch_event(*system, draw_event);

    alia_bump_allocator_commit_peak(&draw_context.arena);

    std::unordered_set<alia_draw_target_id> targets;
    targets.insert(ALIA_DRAW_TARGET_PRIMARY);
    std::vector<std::pair<alia_draw_target_id, alia_draw_target_id>> edges;

    for (auto const key : bucket_table.keys)
    {
        alia_draw_target_id const target = target_from_key(key);
        targets.insert(target);
        if (material_from_key(key) != ALIA_DRAW_TARGET_MATERIAL_ID)
            continue;
        alia_draw_bucket const* bucket = &bucket_table.buckets[key];
        for (auto const* cmd = bucket->head; cmd; cmd = cmd->next)
        {
            auto const* blit = alia::downcast<alia_draw_target_command>(cmd);
            targets.insert(blit->source);
            // Source must be painted before the target that composites it.
            edges.push_back({blit->source, target});
        }
    }

    std::unordered_map<alia_draw_target_id, std::vector<uint64_t>>
        keys_by_target;
    for (auto const key : bucket_table.keys)
        keys_by_target[target_from_key(key)].push_back(key);
    for (auto& entry : keys_by_target)
        std::sort(entry.second.begin(), entry.second.end());

    std::vector<alia_draw_target_id> const order
        = order_draw_targets(targets, edges);

    alia_renderer_ops const& ops = system->renderer;
    if (ops.draw_pass_begin)
        ops.draw_pass_begin(ops.user);

    float const clear_transparent[4] = {0.f, 0.f, 0.f, 0.f};
    for (alia_draw_target_id const target : order)
    {
        if (ops.draw_target_bind)
            ops.draw_target_bind(ops.user, target);
        if (target != ALIA_DRAW_TARGET_PRIMARY && ops.draw_target_clear)
            ops.draw_target_clear(ops.user, target, clear_transparent);

        auto it = keys_by_target.find(target);
        if (it == keys_by_target.end())
            continue;
        for (auto const key : it->second)
        {
            alia_draw_bucket* bucket = &bucket_table.buckets[key];
            alia_draw_material_id const material_id = material_from_key(key);
            alia_draw_material* material
                = &system->draw.materials[material_id];
            material->vtable.draw_bucket(material->user, bucket);
        }
    }

    if (ops.draw_pass_end)
        ops.draw_pass_end(ops.user);
}

} // extern "C"
