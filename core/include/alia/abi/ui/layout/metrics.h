#ifndef ALIA_ABI_UI_LAYOUT_METRICS_H
#define ALIA_ABI_UI_LAYOUT_METRICS_H

#include <alia/abi/base/geometry.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

// Size arguments to the layout ABI (gaps, padding, min/max sizes, leaf
// content) are in logical pixels. They are converted to physical pixels via
// the current geometry scale when the layout tree is built. Placement boxes
// returned by the consume APIs are in physical pixels.

typedef struct alia_layout_content_metrics
{
    // content size in logical pixels
    alia_vec2f size;
    // baseline ascent/descent in logical pixels
    float ascent;
    float descent;
} alia_layout_content_metrics;

static inline alia_layout_content_metrics
alia_layout_content_metrics_make(alia_vec2f size)
{
    return ALIA_BRACED_INIT(alia_layout_content_metrics, size, 0.f, 0.f);
}

ALIA_EXTERN_C_END

#endif // ALIA_ABI_UI_LAYOUT_METRICS_H
