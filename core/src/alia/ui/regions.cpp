#include <alia/abi/ui/regions.h>

#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/context.h>
#include <alia/abi/ui/geometry/api.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/base/stack.hpp>
#include <alia/impl/events.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/ui/regions.h>
#include <alia/ui/system/object.h>
#include <alia/ui/trace_internal.h>

using namespace alia;

namespace {

// parent cursor saved across geometry push/pop during a regions pass
struct regions_nesting_frame
{
    alia_region* current;
    alia_region** next_child;
};

alia_vec2f
content_origin_on_surface(alia_region const* current)
{
    alia_vec2f origin{0.f, 0.f};
    for (alia_region const* node = current; node; node = node->parent)
    {
        if (node->kind == ALIA_REGION_KIND_TRANSLATE)
            origin = alia_vec2f_add(origin, node->translation);
    }
    return origin;
}

bool
passes_z_test(alia_hit_target const& current, alia_z_index candidate_z)
{
    return !alia_element_id_is_valid(current.id) || candidate_z >= current.z;
}

void
consider_hit(
    alia_hit_target& target,
    alia_box* surface_box,
    alia_element_id id,
    alia_z_index z,
    alia_box const& region_surface_box)
{
    if (!passes_z_test(target, z))
        return;
    target.id = id;
    target.z = z;
    if (surface_box)
        *surface_box = region_surface_box;
}

struct hit_slot
{
    alia_region_flags_t required_flags;
    alia_hit_target* target;
    alia_box* surface_box;
};

void
hit_test_node(
    alia_region const* node,
    alia_vec2f point,
    alia_vec2f content_origin,
    hit_slot* slots,
    int slot_count)
{
    ALIA_ASSERT(node);

    switch (node->kind)
    {
        case ALIA_REGION_KIND_ELEMENT: {
            auto const& element = node->element;
            if (!alia_box_contains(element.box, point))
                return;
            alia_box const surface_box
                = alia_box_translate(element.box, content_origin);
            for (int i = 0; i < slot_count; ++i)
            {
                hit_slot& slot = slots[i];
                if ((element.flags & slot.required_flags)
                    != slot.required_flags)
                    continue;
                consider_hit(
                    *slot.target,
                    slot.surface_box,
                    element.id,
                    element.z,
                    surface_box);
            }
            for (alia_region const* child = node->first_child; child;
                 child = child->next)
            {
                hit_test_node(child, point, content_origin, slots, slot_count);
            }
            break;
        }
        case ALIA_REGION_KIND_CLIP: {
            if (!alia_box_contains(node->clip, point))
                return;
            for (alia_region const* child = node->first_child; child;
                 child = child->next)
            {
                hit_test_node(child, point, content_origin, slots, slot_count);
            }
            break;
        }
        case ALIA_REGION_KIND_TRANSLATE: {
            alia_vec2f const child_point
                = alia_vec2f_sub(point, node->translation);
            alia_vec2f const child_origin
                = alia_vec2f_add(content_origin, node->translation);
            for (alia_region const* child = node->first_child; child;
                 child = child->next)
            {
                hit_test_node(
                    child, child_point, child_origin, slots, slot_count);
            }
            break;
        }
        default:
            ALIA_ASSERT(false);
            break;
    }
}

void
hit_test_tree(
    alia_region const* root, float x, float y, hit_slot* slots, int slot_count)
{
    for (int i = 0; i < slot_count; ++i)
    {
        ALIA_ASSERT(slots[i].target);
        *slots[i].target = {.id = {}, .z = 0};
        if (slots[i].surface_box)
            *slots[i].surface_box = {{0.f, 0.f}, {0.f, 0.f}};
    }
    if (!root)
        return;
    hit_test_node(root, {x, y}, {0.f, 0.f}, slots, slot_count);
}

alia_region*
append_region(alia_regions_context* regions, alia_region_kind_t kind)
{
    ALIA_ASSERT(regions);
    ALIA_ASSERT(regions->current);
    ALIA_ASSERT(regions->next_child);

    alia_region* node = arena_alloc<alia_region>(regions->arena);
    node->parent = regions->current;
    node->next = nullptr;
    node->first_child = nullptr;
    node->kind = kind;

    *regions->next_child = node;
    regions->next_child = &node->next;
    return node;
}

void
push_region_container(alia_ui_context* ctx, alia_region* node)
{
    alia_regions_context* regions = alia_ctx_regions(ctx);
    ALIA_ASSERT(regions);
    ALIA_ASSERT(node);

    regions_nesting_frame& saved
        = stack_push<regions_nesting_frame>(alia_ctx_kernel(ctx));
    saved.current = regions->current;
    saved.next_child = regions->next_child;

    regions->current = node;
    regions->next_child = &node->first_child;
}

void
pop_region_container(alia_ui_context* ctx)
{
    alia_regions_context* regions = alia_ctx_regions(ctx);
    ALIA_ASSERT(regions);

    regions_nesting_frame const& saved
        = stack_pop<regions_nesting_frame>(alia_ctx_kernel(ctx));
    regions->current = saved.current;
    regions->next_child = saved.next_child;
}

bool
find_element_region(
    alia_region const* node,
    alia_element_id id,
    alia_vec2f content_origin,
    alia_box* out_surface_box)
{
    ALIA_ASSERT(node);
    ALIA_ASSERT(out_surface_box);

    switch (node->kind)
    {
        case ALIA_REGION_KIND_ELEMENT: {
            if (alia_element_id_equal(node->element.id, id))
            {
                *out_surface_box
                    = alia_box_translate(node->element.box, content_origin);
                return true;
            }
            for (alia_region const* child = node->first_child; child;
                 child = child->next)
            {
                if (find_element_region(
                        child, id, content_origin, out_surface_box))
                    return true;
            }
            return false;
        }
        case ALIA_REGION_KIND_CLIP: {
            for (alia_region const* child = node->first_child; child;
                 child = child->next)
            {
                if (find_element_region(
                        child, id, content_origin, out_surface_box))
                    return true;
            }
            return false;
        }
        case ALIA_REGION_KIND_TRANSLATE: {
            alia_vec2f const child_origin
                = alia_vec2f_add(content_origin, node->translation);
            for (alia_region const* child = node->first_child; child;
                 child = child->next)
            {
                if (find_element_region(
                        child, id, child_origin, out_surface_box))
                    return true;
            }
            return false;
        }
        default:
            ALIA_ASSERT(false);
            return false;
    }
}

} // namespace

namespace alia {

void
regions_push_clip(alia_ui_context* ctx, alia_box absolute_clip)
{
    alia_regions_context* regions = alia_ctx_regions(ctx);
    ALIA_ASSERT(regions);
    ALIA_ASSERT(regions->current);

    alia_vec2f const origin = content_origin_on_surface(regions->current);
    alia_region* node = append_region(regions, ALIA_REGION_KIND_CLIP);
    node->clip
        = {alia_vec2f_sub(absolute_clip.min, origin), absolute_clip.size};
    push_region_container(ctx, node);
}

void
regions_pop_clip(alia_ui_context* ctx)
{
    ALIA_ASSERT(alia_ctx_regions(ctx));
    pop_region_container(ctx);
}

void
regions_push_translation(alia_ui_context* ctx, alia_vec2f delta)
{
    alia_regions_context* regions = alia_ctx_regions(ctx);
    ALIA_ASSERT(regions);

    alia_region* node = append_region(regions, ALIA_REGION_KIND_TRANSLATE);
    node->translation = delta;
    push_region_container(ctx, node);
}

void
regions_pop_translation(alia_ui_context* ctx)
{
    ALIA_ASSERT(alia_ctx_regions(ctx));
    pop_region_container(ctx);
}

} // namespace alia

extern "C" {

void
alia_element_box_region(
    alia_ui_context* ctx,
    alia_element_id id,
    alia_box const* box,
    alia_z_index z_plane,
    alia_region_flags_t flags)
{
    ALIA_ASSERT(ctx);
    ALIA_ASSERT(box);
    alia_regions_context* regions = alia_ctx_regions(ctx);
    if (!regions)
        return;
    if (!alia_element_id_is_valid(id))
        return;

    alia_region* node = append_region(regions, ALIA_REGION_KIND_ELEMENT);
    node->element.id = id;
    node->element.box = *box;
    node->element.z = alia_z(ctx, z_plane);
    node->element.flags = flags;
}

void
alia_ui_rebuild_regions(alia_ui_system* ui)
{
    ALIA_ASSERT(ui);
    alia::trace_pass_builder trace(*ui, ALIA_TRACE_PASS_EVENT);
    alia_event event = alia_make_regions_event({});
    alia::dispatch_event(*ui, event);
    if (trace.active())
    {
        trace.pass.event.type = event.type;
        trace.pass.event.target = event.target;
    }
}

bool
alia_ui_hit_test(
    alia_ui_system const* ui,
    float x,
    float y,
    alia_region_flags_t flags,
    alia_hit_target* out_target,
    alia_box* out_surface_box)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(out_target);
    hit_slot slot{
        .required_flags = flags,
        .target = out_target,
        .surface_box = out_surface_box};
    hit_test_tree(ui->regions.root, x, y, &slot, 1);
    return alia_element_id_is_valid(out_target->id);
}

void
alia_ui_mouse_hit_test(
    alia_ui_system const* ui,
    float x,
    float y,
    alia_mouse_hit_test_result* out)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(out);
    *out = {};
    alia_ui_hit_test(ui, x, y, ALIA_REGION_MOUSE, &out->target, &out->region);
}

void
alia_ui_scroll_input_hit_test(
    alia_ui_system const* ui,
    float x,
    float y,
    alia_scroll_input_hit_test_result* out)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(out);
    *out = {};
    alia_ui_hit_test(
        ui, x, y, ALIA_REGION_SCROLL_INPUT, &out->target, nullptr);
}

void
alia_ui_touch_gesture_hit_test(
    alia_ui_system const* ui,
    float x,
    float y,
    alia_touch_gesture_hit_test_result* out)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(out);
    *out = {};
    hit_slot slots[] = {
        {.required_flags = ALIA_REGION_MOUSE,
         .target = &out->pointer,
         .surface_box = nullptr},
        {.required_flags = ALIA_REGION_SCROLL_INPUT,
         .target = &out->scroll,
         .surface_box = nullptr},
        {.required_flags = ALIA_REGION_TOUCH_DRAG,
         .target = &out->touch_drag,
         .surface_box = nullptr},
    };
    hit_test_tree(ui->regions.root, x, y, slots, 3);
}

bool
alia_ui_scroll_into_view(alia_ui_system* ui, alia_element_id id)
{
    ALIA_ASSERT(ui);
    if (!alia_element_id_is_valid(id))
        return false;
    if (!ui->regions.root)
        return false;

    alia_box surface{};
    if (!find_element_region(ui->regions.root, id, {0.f, 0.f}, &surface))
        return false;

    alia_event event = alia_make_scroll_into_view_event(
        {.acknowledged = true, .region = surface});
    event.target = id;
    alia::dispatch_event(*ui, event);
    return true;
}

} // extern "C"
