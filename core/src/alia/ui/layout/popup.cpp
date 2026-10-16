#include <alia/abi/ui/layout/popup.h>

using namespace alia::operators;

namespace {

alia_vec2f
zero_clamp(alia_vec2f v, alia_vec2f max)
{
    return alia_vec2f_max(alia_vec2f_make(0.f, 0.f), alia_vec2f_min(v, max));
}

struct popup_candidate
{
    bool below;
    bool start;
    alia_vec2f available_size;
};

float
coverage(alia_vec2f preferred, popup_candidate const& candidate)
{
    alia_vec2f const shown_size
        = alia_vec2f_min(preferred, candidate.available_size);
    return shown_size.x * shown_size.y;
}

alia_vec2f
clamped_size(
    alia_vec2f preferred,
    alia_vec2f min_size,
    popup_candidate const& candidate)
{
    return alia_vec2f_min(
        alia_vec2f_max(min_size, preferred), candidate.available_size);
}

} // namespace

extern "C" {

alia_box
alia_popup_place(
    alia_box anchor,
    alia_vec2f preferred_size,
    alia_vec2f min_size,
    alia_box bounds)
{
    alia_vec2f const bounds_size = bounds.size;
    alia_vec2f const bounds_max = bounds.min + bounds_size;
    alia_vec2f const anchor_max = anchor.min + anchor.size;

    popup_candidate const candidates[4] = {
        {true,
         true,
         zero_clamp(
             alia_vec2f_make(
                 bounds_max.x - anchor.min.x, bounds_max.y - anchor_max.y),
             bounds_size)},
        {true,
         false,
         zero_clamp(
             alia_vec2f_make(
                 anchor_max.x - bounds.min.x, bounds_max.y - anchor_max.y),
             bounds_size)},
        {false,
         true,
         zero_clamp(
             alia_vec2f_make(
                 bounds_max.x - anchor.min.x, anchor.min.y - bounds.min.y),
             bounds_size)},
        {false,
         false,
         zero_clamp(
             alia_vec2f_make(
                 anchor_max.x - bounds.min.x, anchor.min.y - bounds.min.y),
             bounds_size)},
    };

    int best = 0;
    float best_score = -1.f;
    for (int i = 0; i != 4; ++i)
    {
        float const score = coverage(preferred_size, candidates[i]);
        if (score > best_score)
        {
            best_score = score;
            best = i;
        }
    }

    popup_candidate const& chosen = candidates[best];
    alia_vec2f const size = clamped_size(preferred_size, min_size, chosen);
    alia_vec2f p = alia_vec2f_make(
        chosen.start ? anchor.min.x : anchor_max.x - size.x,
        chosen.below ? anchor_max.y : anchor.min.y - size.y);
    p = alia_vec2f_max(bounds.min, p);
    if (p.x + size.x > bounds_max.x)
        p.x = bounds_max.x - size.x;
    if (p.y + size.y > bounds_max.y)
        p.y = bounds_max.y - size.y;
    return alia_box_make(p, size);
}

} // extern "C"
