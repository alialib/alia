#include <alia/abi/ui/styling/environment.h>

#include <alia/abi/ui/styling/context_types.h>
#include <alia/abi/ui/styling/descriptors.h>

extern "C" {

alia_style_seeds
alia_style_seeds_default(void)
{
    alia_style_seeds out{};
    float* base = reinterpret_cast<float*>(&out);
    size_t count = 0;
    alia_style_float_desc const* descs = alia_style_seed_descs(&count);
    for (size_t i = 0; i < count; ++i)
    {
        alia_style_float_desc const& d = descs[i];
        // Seeds share one default for light and dark.
        *alia_style_float_at(base, &d) = d.default_light;
    }
    return out;
}

void
alia_style_apply_seeds(
    alia_style_context* style, alia_style_seeds const* seeds)
{
    ALIA_ASSERT(style);
    alia_style_seeds const s = seeds ? *seeds : alia_style_seeds_default();
    style->spacing = s.spacing;
    style->focus.show_on_pointer = false;
    float const ring = 2.f * s.scale;
    style->focus.ring.thickness = ring;
    style->focus.ring.outset = ring;
}

} // extern "C"
