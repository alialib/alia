#include <alia/abi/ui/layout/utilities/line.h>

extern "C" {

alia_layout_line_item_spacing
alia_layout_justify_line(
    alia_layout_flags_t flags, float extra_space, int item_count)
{
    if (extra_space <= 0.f || item_count <= 0)
        return {0.f, 0.f};

    switch (flags & ALIA_JUSTIFY_MASK)
    {
        case ALIA_JUSTIFY_START:
        default:
            return {0.f, 0.f};
        case ALIA_JUSTIFY_END:
            return {extra_space, 0.f};
        case ALIA_JUSTIFY_CENTER:
            return {extra_space * 0.5f, 0.f};
        case ALIA_JUSTIFY_SPACE_BETWEEN:
            if (item_count >= 2)
            {
                return {0.f, extra_space / static_cast<float>(item_count - 1)};
            }
            else
            {
                return {0.f, 0.f};
            }
        case ALIA_JUSTIFY_SPACE_AROUND: {
            float const gap = extra_space / static_cast<float>(item_count);
            return {gap * 0.5f, gap};
        }
        case ALIA_JUSTIFY_SPACE_EVENLY: {
            float const gap = extra_space / static_cast<float>(item_count + 1);
            return {gap, gap};
        }
    }
}

} // extern "C"
