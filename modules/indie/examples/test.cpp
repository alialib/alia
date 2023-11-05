#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/indie.hpp>
#include <alia/indie/layout/containers/simple.hpp>
#include <alia/indie/layout/containers/utilities.hpp>
#include <alia/indie/layout/library.hpp>
#include <alia/indie/layout/logic/flow.hpp>
#include <alia/indie/layout/logic/linear.hpp>
#include <alia/indie/layout/spacer.hpp>
#include <alia/indie/layout/utilities.hpp>
#include <alia/indie/system/api.hpp>
#include <alia/indie/system/input_constants.hpp>
#include <alia/indie/utilities/hit_testing.hpp>
#include <alia/indie/utilities/keyboard.hpp>
#include <alia/indie/utilities/mouse.hpp>
#include <alia/indie/utilities/scrolling.hpp>
#include <alia/indie/widget.hpp>

#include <color/color.hpp>

#include <cmath>

#include <include/core/SkColor.h>
#include <include/core/SkFontTypes.h>
#include <include/core/SkMaskFilter.h>
#include <include/core/SkPictureRecorder.h>
#include <include/core/SkTextBlob.h>
#include <limits>
#include <modules/skshaper/include/SkShaper.h>

#include "alia/core/flow/components.hpp"
#include "alia/core/flow/top_level.hpp"
#include "alia/core/signals/core.hpp"
#include "alia/core/timing/ticks.hpp"
#include "alia/indie/context.hpp"
#include "alia/indie/geometry.hpp"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/TextStyle.h"
#include "modules/skparagraph/src/ParagraphBuilderImpl.h"
#include "modules/skparagraph/src/ParagraphImpl.h"

#include <alia/indie/scrolling.hpp>

using namespace alia;
using namespace alia::indie;

// std::unique_ptr<SkShaper> the_shaper;

// TODO
static std::unique_ptr<SkFont> the_font;

struct click_event : targeted_event
{
};

// smooth_raw(ctx, smoother, x, transition) returns a smoothed view of x.
template<class Value>
Value
smooth_position(
    value_smoother<Value>& smoother,
    Value const& x,
    millisecond_count tick_counter,
    animated_transition const& transition = default_transition)
{
    if (!smoother.initialized)
        reset_smoothing(smoother, x);
    Value current_value = smoother.new_value;
    if (smoother.in_transition)
    {
        auto ticks_left = millisecond_count(
            (std::max)(0, int(smoother.transition_end - tick_counter)));
        if (ticks_left > 0)
        {
            double fraction = eval_curve_at_x(
                transition.curve,
                1. - double(ticks_left) / smoother.duration,
                1. / smoother.duration);
            current_value = interpolate(
                smoother.old_value, smoother.new_value, fraction);
        }
        else
            smoother.in_transition = false;
    }
    if (x != smoother.new_value)
    {
        smoother.duration =
            // If we're just going back to the old value, go back in the same
            // amount of time it took to get here.
            smoother.in_transition && x == smoother.old_value
                ? (transition.duration
                   - millisecond_count((std::max)(
                       0, int(smoother.transition_end - tick_counter))))
                : transition.duration;
        smoother.transition_end = tick_counter + smoother.duration;
        smoother.old_value = current_value;
        smoother.new_value = x;
        smoother.in_transition = true;
    }
    return current_value;
}

struct box_node : indie::widget
{
    layout_requirements
    get_horizontal_requirements() override
    {
        layout_requirements requirements;
        resolve_requirements(
            requirements,
            resolved_spec_,
            0,
            calculated_layout_requirements{40, 0, 0});
        return requirements;
    }
    layout_requirements
    get_vertical_requirements(layout_scalar /*assigned_width*/) override
    {
        layout_requirements requirements;
        resolve_requirements(
            requirements,
            resolved_spec_,
            1,
            calculated_layout_requirements{40, 0, 0});
        return requirements;
    }
    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        layout_requirements horizontal_requirements, vertical_requirements;
        resolve_requirements(
            horizontal_requirements,
            resolved_spec_,
            0,
            calculated_layout_requirements{40, 0, 0});
        resolve_requirements(
            vertical_requirements,
            resolved_spec_,
            1,
            calculated_layout_requirements{40, 0, 0});
        relative_assignment_ = resolve_relative_assignment(
            resolved_spec_,
            assignment,
            horizontal_requirements,
            vertical_requirements);
    }

    void
    render(render_event& event) override
    {
        SkCanvas& canvas = *event.canvas;

        auto const& region = this->assignment().region;

        double blend_factor = 0;

        if (indie::is_click_in_progress(
                *sys_,
                internal_element_ref{*this, 0},
                indie::mouse_button::LEFT)
            || is_pressed(keyboard_click_state_))
        {
            blend_factor = 0.4;
        }
        else if (is_click_possible(*sys_, internal_element_ref{*this, 0}))
        {
            blend_factor = 0.2;
        }

        ::color::rgb<std::uint8_t> c;
        if (state_)
        {
            c = ::color::rgb<std::uint8_t>({0x40, 0x40, 0x40});
        }
        else
        {
            c = ::color::rgb<std::uint8_t>(
                {SkColorGetR(color_),
                 SkColorGetG(color_),
                 SkColorGetB(color_)});
        }
        if (blend_factor != 0)
        {
            ::color::yiq<std::uint8_t> color;
            color = c;
            ::color::yiq<std::uint8_t> white = ::color::constant::white_t{};
            ::color::yiq<std::uint8_t> mix
                = ::color::operation::mix(color, blend_factor, white);
            c = mix;
        }

        // auto position = smooth_position(
        //     position_,
        //     region.corner + event.current_offset,
        //     tick_counter_,
        //     {default_curve, 80});
        auto position = region.corner + event.current_offset;

        SkPaint paint;
        paint.setColor(SkColorSetARGB(
            0xff,
            ::color::get::red(c),
            ::color::get::green(c),
            ::color::get::blue(c)));
        SkRect rect;
        rect.fLeft = SkScalar(position[0]);
        rect.fTop = SkScalar(position[1]);
        rect.fRight = SkScalar(position[0] + region.size[0]);
        rect.fBottom = SkScalar(position[1] + region.size[1]);
        canvas.drawRect(rect, paint);

        if (rect.width() > 200)
        {
            SkPaint blur(paint);
            blur.setAlpha(200);
            blur.setMaskFilter(
                SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, 40, false));
            canvas.drawRect(rect, blur);
        }

        if (indie::element_has_focus(*sys_, internal_element_ref{*this, 0}))
        {
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(4);
            paint.setColor(SK_ColorBLACK);
            canvas.drawRect(rect, paint);
        }
    }

    void
    hit_test(indie::hit_test_base& test, vector<2, double> const& point)
        const override
    {
        if (is_inside(this->assignment().region, vector<2, float>(point)))
        {
            if (test.type == indie::hit_test_type::MOUSE)
            {
                static_cast<indie::mouse_hit_test&>(test).result
                    = indie::mouse_hit_test_result{
                        externalize(internal_element_ref{*this, 0}),
                        indie::mouse_cursor::POINTER,
                        this->assignment().region,
                        ""};
            }
        }
    }

    void
    process_input(indie::event_context ctx) override
    {
        indie::add_to_focus_order(ctx, internal_element_ref{*this, 0});
        if (detect_click(
                ctx,
                internal_element_ref{*this, 0},
                indie::mouse_button::LEFT))
        {
            click_event event;
            dispatch_targeted_event(*sys_, event, this->id_);
            state_ = !state_;
            // advance_focus(get_system(ctx));
        }
        // if (detect_key_press(ctx, this, indie::key_code::SPACE))
        // {
        //     state_ = !state_;
        //     // advance_focus(get_system(ctx));
        // }
        if (detect_keyboard_click(
                ctx, keyboard_click_state_, internal_element_ref{*this, 0}))
        {
            state_ = !state_;
        }
    }

    matrix<3, 3, double>
    transformation() const override
    {
        return parent->transformation();
    }

    relative_layout_assignment const&
    assignment() const
    {
        return relative_assignment_;
    }

    // external_component_id
    // identity() const
    // {
    //     return id_;
    // }

    indie::system* sys_;
    external_component_id id_;
    bool state_ = false;
    SkColor color_ = SK_ColorWHITE;
    indie::keyboard_click_state keyboard_click_state_;
    // value_smoother<layout_vector> position_;
    // TODO: Move this into the system.
    // sk_sp<SkPicture> picture_;
    // the resolved spec
    resolved_layout_spec resolved_spec_;
    // resolved relative assignment
    relative_layout_assignment relative_assignment_;
};

void
do_box(
    indie::context ctx,
    SkColor color,
    action<> on_click,
    layout const& layout_spec = layout(TOP | LEFT | PADDED))
{
    std::shared_ptr<box_node>* node_ptr;
    if (get_cached_data(ctx, &node_ptr))
    {
        *node_ptr = std::make_shared<box_node>();
        (*node_ptr)->sys_ = &get_system(ctx);
        (*node_ptr)->color_ = color;
    }
    auto& node = **node_ptr;
    // box_node* node_ptr;
    // if (get_cached_data(ctx, &node_ptr))
    // {
    //     node_ptr->sys_ = &get_system(ctx);
    //     node_ptr->color_ = color;
    // }
    // auto& node = *node_ptr;

    auto id = get_component_id(ctx);

    if (is_refresh_event(ctx))
    {
        resolved_layout_spec resolved_spec;
        resolve_layout_spec(
            get<indie::traversal_tag>(ctx).layout,
            resolved_spec,
            layout_spec,
            TOP | LEFT | PADDED);
        detect_layout_change(
            get<indie::traversal_tag>(ctx).layout,
            &node.resolved_spec_,
            resolved_spec);

        add_layout_node(get<indie::traversal_tag>(ctx).layout, &node);

        node.id_ = externalize(id);
        // node.tick_counter_ = get_raw_animation_tick_count(ctx);

        // if (color != node.color_)
        // {
        //     SkPictureRecorder recorder;
        //     SkRect bounds;
        //     bounds.fLeft = 0;
        //     bounds.fTop = 0;
        //     bounds.fRight = 100;
        //     bounds.fBottom = 100;
        //     SkCanvas* canvas = recorder.beginRecording(bounds);

        //     {
        //         SkPaint paint;
        //         paint.setColor(color);
        //         SkRect rect;
        //         rect.fLeft = 0;
        //         rect.fTop = 0;
        //         rect.fRight = 100;
        //         rect.fBottom = 100;
        //         canvas->drawRect(rect, paint);
        //     }

        //     node.picture_ = recorder.finishRecordingAsPicture();
        // }
    }
    click_event* click;
    if (detect_targeted_event(ctx, id, &click))
    {
        perform_action(on_click);
        abort_traversal(ctx);
    }
}

struct text_node : indie::layout_leaf
{
    void
    render(render_event& event) override
    {
        SkCanvas& canvas = *event.canvas;

        auto const& region = this->assignment().region;

        SkRect bounds;
        bounds.fLeft = SkScalar(region.corner[0] + event.current_offset[0]);
        bounds.fTop = SkScalar(region.corner[1] + event.current_offset[1]);
        bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
        bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
        if (canvas.quickReject(bounds))
            return;
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SK_ColorBLACK);
        // paragraph->paint(&canvas, region.corner[0], region.corner[1]);
        // canvas.drawColor(SK_ColorBLACK);
        if (!text_blob_)
            text_blob_ = SkTextBlob::MakeFromString(text_.c_str(), *the_font);
        canvas.drawTextBlob(
            text_blob_.get(), bounds.fLeft, bounds.fBottom, paint);
    }

    void
    hit_test(indie::hit_test_base&, vector<2, double> const&) const override
    {
    }

    void
    process_input(indie::event_context) override
    {
    }

    matrix<3, 3, double>
    transformation() const override
    {
        return parent->transformation();
    }

    indie::system* sys_;
    // std::unique_ptr<skia::textlayout::Paragraph> paragraph;
    std::string text_;
    SkRect bounds_;
    sk_sp<SkTextBlob> text_blob_;
};

void
do_text(
    indie::context ctx,
    readable<std::string> text,
    layout const& layout_spec = default_layout)
{
    if (!the_font)
        the_font.reset(new SkFont(nullptr, 24));

    text_node* node_ptr;
    if (get_cached_data(ctx, &node_ptr))
    {
        node_ptr->sys_ = &get_system(ctx);

#if 0
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SK_ColorBLACK);
        skia::textlayout::TextStyle defaultStyle;
        defaultStyle.setForegroundColor(paint);
        skia::textlayout::ParagraphStyle paraStyle;

        defaultStyle.setFontSize(SkScalar(24));
        paraStyle.setTextStyle(defaultStyle);
        skia::textlayout::ParagraphBuilderImpl builder(
            paraStyle, get_system(ctx).font_collection);

        skia::textlayout::TextStyle style;
        style.setFontFamilies({SkString("Roboto")});
        SkFontStyle fontStyle(
            false ? SkFontStyle::Weight::kBold_Weight
                  : SkFontStyle::Weight::kNormal_Weight,
            SkFontStyle::Width::kNormal_Width,
            false ? SkFontStyle::Slant::kItalic_Slant
                  : SkFontStyle::Slant::kUpright_Slant);
        style.setForegroundColor(paint);
        style.setFontStyle(fontStyle);
        style.setFontSize(24);
        // if (false)
        // {
        //     style.addShadow(skia::textlayout::TextShadow(
        //         SK_ColorBLACK, SkPoint::Make(5, 5), 2));
        // }
        // bool test
        //     = (skia::textlayout::TextDecoration) 0
        //       != skia::textlayout::TextDecoration::kNoDecoration;
        // if (test)
        // {
        //     style.setDecoration((TextDecoration) decoration);
        //     style.setDecorationStyle(std::get<7>(para));
        //     style.setDecorationColor(std::get<5>(para));
        // }
        builder.pushStyle(style);
        builder.addText(read_signal(text).c_str());
        builder.pop();

        (*node_ptr)->paragraph = builder.Build();
        (*node_ptr)->paragraph->layout(
            std::numeric_limits<SkScalar>::infinity());
#else
        node_ptr->text_ = read_signal(text);
        the_font->measureText(
            read_signal(text).c_str(),
            read_signal(text).length(),
            SkTextEncoding::kUTF8,
            &node_ptr->bounds_);
#endif
    }

    auto& node = *node_ptr;

    // auto id = get_component_id(ctx);

    if (is_refresh_event(ctx))
    {
        node.refresh_layout(
            get<indie::traversal_tag>(ctx).layout,
            layout_spec,
            leaf_layout_requirements(
                make_layout_vector(
                    node.bounds_.width(), node.bounds_.height()),
                0,
                0));
        add_layout_node(get<indie::traversal_tag>(ctx).layout, &node);

        // node.id_ = externalize(id);

        // if (color != node.color_)
        // {
        //     SkPictureRecorder recorder;
        //     SkRect bounds;
        //     bounds.fLeft = 0;
        //     bounds.fTop = 0;
        //     bounds.fRight = 100;
        //     bounds.fBottom = 100;
        //     SkCanvas* canvas = recorder.beginRecording(bounds);

        //     {
        //         SkPaint paint;
        //         paint.setColor(color);
        //         SkRect rect;
        //         rect.fLeft = 0;
        //         rect.fTop = 0;
        //         rect.fRight = 100;
        //         rect.fBottom = 100;
        //         canvas->drawRect(rect, paint);
        //     }

        //     node.picture_ = recorder.finishRecordingAsPicture();
        // }
    }
}

// ---

#pragma warning(push, 0)

class RunHandler final : public SkShaper::RunHandler
{
 public:
    RunHandler(const char* utf8Text, size_t) : fUtf8Text(utf8Text)
    {
    }
    using RunCallback = void (*)(
        void* context,
        const char* utf8Text,
        size_t utf8TextBytes,
        size_t glyphCount,
        const SkGlyphID* glyphs,
        const SkPoint* positions,
        const uint32_t* clusters,
        const SkFont& font);
    void
    setRunCallback(RunCallback f, void* context)
    {
        fCallbackContext = context;
        fCallbackFunction = f;
    }
    sk_sp<SkTextBlob>
    makeBlob();
    SkPoint
    endPoint() const
    {
        return fOffset;
    }
    SkPoint
    finalPosition() const
    {
        return fCurrentPosition;
    }
    void
    beginLine() override;
    void
    runInfo(const RunInfo&) override;
    void
    commitRunInfo() override;
    SkShaper::RunHandler::Buffer
    runBuffer(const RunInfo&) override;
    void
    commitRunBuffer(const RunInfo&) override;
    void
    commitLine() override;
    const std::vector<size_t>&
    lineEndOffsets() const
    {
        return fLineEndOffsets;
    }
    SkRect
    finalRect(const SkFont& font) const
    {
        if (0 == fMaxRunAscent || 0 == fMaxRunDescent)
        {
            SkFontMetrics metrics;
            font.getMetrics(&metrics);
            return {
                fCurrentPosition.x(),
                fCurrentPosition.y(),
                fCurrentPosition.x() + font.getSize(),
                fCurrentPosition.y() + metrics.fDescent - metrics.fAscent};
        }
        else
        {
            return {
                fCurrentPosition.x(),
                fCurrentPosition.y() + fMaxRunAscent,
                fCurrentPosition.x() + font.getSize(),
                fCurrentPosition.y() + fMaxRunDescent};
        }
    }

 private:
    SkTextBlobBuilder fBuilder;
    std::vector<size_t> fLineEndOffsets;
    const SkGlyphID* fCurrentGlyphs = nullptr;
    const SkPoint* fCurrentPoints = nullptr;
    void* fCallbackContext = nullptr;
    RunCallback fCallbackFunction = nullptr;
    char const* const fUtf8Text;
    size_t fTextOffset = 0;
    uint32_t* fClusters = nullptr;
    int fClusterOffset = 0;
    int fGlyphCount = 0;
    SkScalar fMaxRunAscent = 0;
    SkScalar fMaxRunDescent = 0;
    SkScalar fMaxRunLeading = 0;
    SkPoint fCurrentPosition = {0, 0};
    SkPoint fOffset = {0, 0};
};

void
RunHandler::beginLine()
{
    fCurrentPosition = fOffset;
    fMaxRunAscent = 0;
    fMaxRunDescent = 0;
    fMaxRunLeading = 0;
}
void
RunHandler::runInfo(const SkShaper::RunHandler::RunInfo& info)
{
    SkFontMetrics metrics;
    info.fFont.getMetrics(&metrics);
    fMaxRunAscent = std::min(fMaxRunAscent, metrics.fAscent);
    fMaxRunDescent = std::max(fMaxRunDescent, metrics.fDescent);
    fMaxRunLeading = std::max(fMaxRunLeading, metrics.fLeading);
}
void
RunHandler::commitRunInfo()
{
    fCurrentPosition.fY -= fMaxRunAscent;
}
SkShaper::RunHandler::Buffer
RunHandler::runBuffer(const RunInfo& info)
{
    int glyphCount
        = SkTFitsIn<int>(info.glyphCount) ? info.glyphCount : INT_MAX;
    int utf8RangeSize = SkTFitsIn<int>(info.utf8Range.size())
                            ? info.utf8Range.size()
                            : INT_MAX;
    const auto& runBuffer
        = fBuilder.allocRunTextPos(info.fFont, glyphCount, utf8RangeSize);
    fCurrentGlyphs = runBuffer.glyphs;
    fCurrentPoints = runBuffer.points();
    if (runBuffer.utf8text && fUtf8Text)
    {
        memcpy(
            runBuffer.utf8text,
            fUtf8Text + info.utf8Range.begin(),
            utf8RangeSize);
    }
    fClusters = runBuffer.clusters;
    fGlyphCount = glyphCount;
    fClusterOffset = info.utf8Range.begin();
    return {
        runBuffer.glyphs,
        runBuffer.points(),
        nullptr,
        runBuffer.clusters,
        fCurrentPosition};
}
void
RunHandler::commitRunBuffer(const RunInfo& info)
{
    // for (size_t i = 0; i < info.glyphCount; ++i) {
    //     SkASSERT(fClusters[i] >= info.utf8Range.begin());
    //     // this fails for khmer example.
    //     SkASSERT(fClusters[i] <  info.utf8Range.end());
    // }
    if (fCallbackFunction)
    {
        fCallbackFunction(
            fCallbackContext,
            fUtf8Text,
            info.utf8Range.end(),
            info.glyphCount,
            fCurrentGlyphs,
            fCurrentPoints,
            fClusters,
            info.fFont);
    }
    SkASSERT(0 <= fClusterOffset);
    for (int i = 0; i < fGlyphCount; ++i)
    {
        SkASSERT(fClusters[i] >= (unsigned) fClusterOffset);
        fClusters[i] -= fClusterOffset;
    }
    fCurrentPosition += info.fAdvance;
    fTextOffset = std::max(fTextOffset, info.utf8Range.end());
}
void
RunHandler::commitLine()
{
    if (fLineEndOffsets.empty() || fTextOffset > fLineEndOffsets.back())
    {
        // Ensure that fLineEndOffsets is monotonic.
        fLineEndOffsets.push_back(fTextOffset);
    }
    fOffset += {0, fMaxRunDescent + fMaxRunLeading - fMaxRunAscent};
}
sk_sp<SkTextBlob>
RunHandler::makeBlob()
{
    return fBuilder.make();
}
static SkRect
selection_box(const SkFontMetrics& metrics, float advance, SkPoint pos)
{
    if (fabsf(advance) < 1.0f)
    {
        advance = copysignf(1.0f, advance);
    }
    return SkRect{
        pos.x(),
        pos.y() + metrics.fAscent,
        pos.x() + advance,
        pos.y() + metrics.fDescent}
        .makeSorted();
}
static void
set_character_bounds(
    void* context,
    const char* utf8Text,
    size_t utf8TextBytes,
    size_t glyphCount,
    const SkGlyphID* glyphs,
    const SkPoint* positions,
    const uint32_t* clusters,
    const SkFont& font)
{
    SkASSERT(context);
    SkASSERT(glyphCount > 0);
    SkRect* cursors = (SkRect*) context;
    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    std::unique_ptr<float[]> advances(new float[glyphCount]);
    font.getWidths(glyphs, glyphCount, advances.get());
    // Loop over each cluster in this run.
    size_t clusterStart = 0;
    for (size_t glyphIndex = 0; glyphIndex < glyphCount; ++glyphIndex)
    {
        if (glyphIndex + 1 < glyphCount // more glyphs
            && clusters[glyphIndex] == clusters[glyphIndex + 1])
        {
            continue; // multi-glyph cluster
        }
        unsigned textBegin = clusters[glyphIndex];
        unsigned textEnd = utf8TextBytes;
        for (size_t i = 0; i < glyphCount; ++i)
        {
            if (clusters[i] >= textEnd)
            {
                textEnd = clusters[i] + 1;
            }
        }
        for (size_t i = 0; i < glyphCount; ++i)
        {
            if (clusters[i] > textBegin && clusters[i] < textEnd)
            {
                textEnd = clusters[i];
                if (textEnd == textBegin + 1)
                {
                    break;
                }
            }
        }
        SkASSERT(glyphIndex + 1 > clusterStart);
        unsigned clusterGlyphCount = glyphIndex + 1 - clusterStart;
        const SkPoint* clusterGlyphPositions = &positions[clusterStart];
        const float* clusterAdvances = &advances[clusterStart];
        clusterStart = glyphIndex + 1; // for next loop
        SkRect clusterBox = selection_box(
            metrics, clusterAdvances[0], clusterGlyphPositions[0]);
        for (unsigned i = 1; i < clusterGlyphCount; ++i)
        { // multiple glyphs
            clusterBox.join(selection_box(
                metrics, clusterAdvances[i], clusterGlyphPositions[i]));
        }
        if (textBegin + 1 == textEnd)
        { // single byte, fast path.
            cursors[textBegin] = clusterBox;
            continue;
        }
        int textCount = textEnd - textBegin;
        int codePointCount = SkUTF::CountUTF8(utf8Text + textBegin, textCount);
        if (codePointCount == 1)
        { // single codepoint, fast path.
            cursors[textBegin] = clusterBox;
            continue;
        }
        float width = clusterBox.width() / codePointCount;
        SkASSERT(width > 0);
        const char* ptr = utf8Text + textBegin;
        const char* end = utf8Text + textEnd;
        float x = clusterBox.left();
        while (ptr < end)
        { // for each codepoint in cluster
            const char* nextPtr = ptr;
            SkUTF::NextUTF8(&nextPtr, end);
            int firstIndex = ptr - utf8Text;
            float nextX = x + width;
            cursors[firstIndex]
                = SkRect{x, clusterBox.top(), nextX, clusterBox.bottom()};
            x = nextX;
            ptr = nextPtr;
        }
    }
}

struct ShapeResult
{
    sk_sp<SkTextBlob> blob;
    std::vector<std::size_t> lineBreakOffsets;
    std::vector<SkRect> glyphBounds;
    int verticalAdvance;
};

ShapeResult
Shape(
    const char* utf8Text, size_t textByteLen, const SkFont& font, float width)
{
    ShapeResult result;
    if (SkUTF::CountUTF8(utf8Text, textByteLen) < 0)
    {
        utf8Text = nullptr;
        textByteLen = 0;
    }
    std::unique_ptr<SkShaper> shaper = SkShaper::Make();
    float height = font.getSpacing();
    RunHandler runHandler(utf8Text, textByteLen);
    if (textByteLen)
    {
        result.glyphBounds.resize(textByteLen);
        for (SkRect& c : result.glyphBounds)
        {
            c = SkRect{-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};
        }
        runHandler.setRunCallback(
            set_character_bounds, result.glyphBounds.data());
        // TODO: make use of locale in shaping.
        shaper->shape(utf8Text, textByteLen, font, true, width, &runHandler);
        if (runHandler.lineEndOffsets().size() > 1)
        {
            result.lineBreakOffsets = runHandler.lineEndOffsets();
            SkASSERT(result.lineBreakOffsets.size() > 0);
            result.lineBreakOffsets.pop_back();
        }
        height = std::max(height, runHandler.endPoint().y());
        result.blob = runHandler.makeBlob();
    }
    result.glyphBounds.push_back(runHandler.finalRect(font));
    result.verticalAdvance = (int) ceilf(height);
    return result;
}

// ---

struct wrapped_text_node : indie::widget
{
    void
    render(render_event& event) override
    {
        SkCanvas& canvas = *event.canvas;

        auto const& region = this->relative_assignment_.region;

        SkRect bounds;
        bounds.fLeft = SkScalar(region.corner[0] + event.current_offset[0]);
        bounds.fTop = SkScalar(region.corner[1] + event.current_offset[1]);
        bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
        bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
        if (canvas.quickReject(bounds))
            return;

        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SK_ColorBLACK);

        canvas.drawTextBlob(
            shape_.blob.get(), bounds.fLeft, bounds.fTop, paint);
    }

    void
    hit_test(indie::hit_test_base&, vector<2, double> const&) const override
    {
    }

    void
    process_input(indie::event_context) override
    {
    }

    matrix<3, 3, double>
    transformation() const override
    {
        return parent->transformation();
    }

    layout_requirements
    get_horizontal_requirements() override
    {
        layout_requirements requirements;
        resolve_requirements(
            requirements,
            resolved_spec_,
            0,
            // TODO: What should the actual minimum width be here?
            calculated_layout_requirements{12, 0, 0});
        return requirements;
    }
    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override
    {
        if (shape_width_ != assigned_width)
        {
            shape_ = Shape(
                text_.c_str(), text_.size(), *the_font, assigned_width);
            shape_width_ = assigned_width;
        }

        layout_requirements requirements;
        resolve_requirements(
            requirements,
            resolved_spec_,
            1,
            calculated_layout_requirements{
                layout_scalar(shape_.verticalAdvance),
                0 /* TODO: ascent */,
                0 /* TODO: descent */});
        return requirements;
    }
    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        layout_requirements horizontal_requirements;
        resolve_requirements(
            horizontal_requirements,
            resolved_spec_,
            0,
            // TODO: What should the actual minimum width be here?
            calculated_layout_requirements{12, 0, 0});

        if (shape_width_ != assignment.region.size[0])
        {
            shape_ = Shape(
                text_.c_str(),
                text_.size(),
                *the_font,
                assignment.region.size[0]);
            shape_width_ = assignment.region.size[0];
        }

        layout_requirements vertical_requirements;
        resolve_requirements(
            vertical_requirements,
            resolved_spec_,
            1,
            calculated_layout_requirements{
                layout_scalar(shape_.verticalAdvance),
                0 /* TODO: ascent */,
                0 /* TODO: descent */});

        relative_assignment_ = resolve_relative_assignment(
            resolved_spec_,
            assignment,
            horizontal_requirements,
            vertical_requirements);
    }

    indie::system* sys_;

    captured_id text_id_;
    std::string text_;

    double shape_width_ = 0;
    ShapeResult shape_;

    // the resolved spec
    resolved_layout_spec resolved_spec_;
    // resolved relative assignment
    relative_layout_assignment relative_assignment_;
};

void
do_wrapped_text(
    indie::context ctx,
    readable<std::string> text,
    layout const& layout_spec = default_layout)
{
    if (!the_font)
        the_font.reset(new SkFont(nullptr, 24));

    wrapped_text_node* node_ptr;
    if (get_cached_data(ctx, &node_ptr))
    {
        node_ptr->sys_ = &get_system(ctx);

        node_ptr->text_ = read_signal(text);
    }

    auto& node = *node_ptr;

    if (is_refresh_event(ctx))
    {
        // TODO: Cache this?
        resolved_layout_spec resolved_spec;
        resolve_layout_spec(
            get_layout_traversal(ctx),
            resolved_spec,
            layout_spec,
            TOP | LEFT | PADDED);
        detect_layout_change(
            get_layout_traversal(ctx), &node.resolved_spec_, resolved_spec);

        refresh_signal_view(
            node.text_id_,
            text,
            [&](std::string const& new_value) { node.text_ = new_value; },
            [&]() { node.text_.clear(); });

        add_layout_node(get<indie::traversal_tag>(ctx).layout, &node);
    }
}

// namespace alia {

// template<class Value>
// struct readable_signal_type
// {
//     using type = readable<Value>;
// };

// template<class Value>
// struct in : readable_signal_type<Value>::type
// {
//     using signal_ref::signal_ref;
// };

// } // namespace alia

// #define ALIA_PP_CONCAT(a, b) ALIA_PP_CONCAT1(a, b)
// #define ALIA_PP_CONCAT1(a, b) ALIA_PP_CONCAT2(a, b)
// #define ALIA_PP_CONCAT2(a, b) a##b

// #define ALIA_PP_FE_2_0(F, a, b)
// #define ALIA_PP_FE_2_1(F, a, b, x) F(a, b, x)
// #define ALIA_PP_FE_2_2(F, a, b, x, ...) \
//     F(a, b, x) ALIA_PP_FE_2_1(F, a, b, __VA_ARGS__)
// #define ALIA_PP_FE_2_3(F, a, b, x, ...) \
//     F(a, b, x) ALIA_PP_FE_2_2(F, a, b, __VA_ARGS__)
// #define ALIA_PP_FE_2_4(F, a, b, x, ...) \
//     F(a, b, x) ALIA_PP_FE_2_3(F, a, b, __VA_ARGS__)
// #define ALIA_PP_FE_2_5(F, a, b, x, ...) \
//     F(a, b, x) ALIA_PP_FE_2_4(F, a, b, __VA_ARGS__)
// #define ALIA_PP_FE_2_6(F, a, b, x, ...) \
//     F(a, b, x) ALIA_PP_FE_2_5(F, a, b, __VA_ARGS__)

// #define ALIA_PP_GET_MACRO(_0, _1, _2, _3, _4, _5, _6, NAME, ...) NAME
// #define ALIA_PP_FOR_EACH_2(F, a, b, ...) \
//     ALIA_PP_GET_MACRO( \
//         _0, \
//         __VA_ARGS__, \
//         ALIA_PP_FE_2_6, \
//         ALIA_PP_FE_2_5, \
//         ALIA_PP_FE_2_4, \
//         ALIA_PP_FE_2_3, \
//         ALIA_PP_FE_2_2, \
//         ALIA_PP_FE_2_1, \
//         ALIA_PP_FE_2_0) \
//     (F, a, b, __VA_ARGS__)

// #define ALIA_DEFINE_STRUCT_SIGNAL_FIELD(signal_type, struct_name, field_name) \
//     auto ALIA_PP_CONCAT(ALIA_PP_CONCAT(_get_, field_name), _signal)()         \
//     {                                                                         \
//         return (*this)->*&struct_name::field_name;                            \
//     }                                                                         \
//     __declspec(property(                                                      \
//         get = ALIA_PP_CONCAT(ALIA_PP_CONCAT(_get_, field_name), _signal)))    \
//         alia::field_signal<                                                   \
//             ALIA_PP_CONCAT(ALIA_PP_CONCAT(signal_type, _), struct_name),      \
//             decltype(struct_name::field_name)>                                \
//             field_name;

// #define ALIA_DEFINE_STRUCT_SIGNAL_FIELDS(signal_type, struct_name, ...)       \
//     ALIA_PP_FOR_EACH_2(                                                       \
//         ALIA_DEFINE_STRUCT_SIGNAL_FIELD,                                      \
//         signal_type,                                                          \
//         struct_name,                                                          \
//         __VA_ARGS__)

// #define ALIA_DEFINE_CUSTOM_STRUCT_SIGNAL(                                     \
//     signal_name, signal_type, struct_name, ...)                               \
//     struct ALIA_PP_CONCAT(ALIA_PP_CONCAT(signal_type, _), struct_name)        \
//         : signal_type<struct_name>                                            \
//     {                                                                         \
//         using signal_ref::signal_ref;                                         \
//         ALIA_DEFINE_STRUCT_SIGNAL_FIELDS(                                     \
//             signal_type, struct_name, __VA_ARGS__)                            \
//     };

// #define ALIA_DEFINE_STRUCT_SIGNAL(signal_type, struct_name, ...)              \
//     ALIA_DEFINE_CUSTOM_STRUCT_SIGNAL(                                         \
//         ALIA_PP_CONCAT(ALIA_PP_CONCAT(signal_type, _), struct_name),          \
//         signal_type,                                                          \
//         struct_name,                                                          \
//         __VA_ARGS__)

// #define ALIA_DEFINE_STRUCT_SIGNALS(struct_name, ...)                          \
//     ALIA_DEFINE_STRUCT_SIGNAL(readable, struct_name, __VA_ARGS__)             \
//     ALIA_DEFINE_STRUCT_SIGNAL(duplex, struct_name, __VA_ARGS__)

// struct realm
// {
//     std::string name;
//     std::string description;
// };
// ALIA_DEFINE_STRUCT_SIGNALS(realm, name, description)

// template<>
// struct alia::readable_signal_type<realm>
// {
//     using type = readable_realm;
// };

// in<realm>
// get_realm_name(indie::context ctx)
// {
//     auto foo = value("focus");
//     return alia::apply(
//         ctx,
//         [] {
//             return realm{foo, "a realm for " + foo};
//         },
//         foo);
// }

namespace alia { namespace indie {

struct scoped_flow_layout : simple_scoped_layout<flow_layout_logic>
{
    using simple_scoped_layout::simple_scoped_layout;

    void
    begin(context ctx, layout const& layout_spec = default_layout)
    {
        // With a flow layout, we want to have the layout itself
        // always fill the horizontal space and use the requested X
        // alignment to position the individual rows in the flow.
        auto adjusted_layout_spec = add_default_padding(layout_spec, PADDED);
        layout_flag_set x_alignment = FILL_X;
        if ((layout_spec.flags.code & 0x3) != 0)
        {
            x_alignment.code
                = adjusted_layout_spec.flags.code & X_ALIGNMENT_MASK_CODE;
            adjusted_layout_spec.flags.code &= ~X_ALIGNMENT_MASK_CODE;
            adjusted_layout_spec.flags.code |= FILL_X_CODE;
        }
        flow_layout_logic* logic;
        get_simple_layout_container(
            get_layout_traversal(ctx),
            get_data_traversal(ctx),
            &container_,
            &logic,
            adjusted_layout_spec);
        logic->x_alignment = x_alignment;
        slc_.begin(get_layout_traversal(ctx), container_);
    }
};

struct nonuniform_grid_tag
{
};
struct uniform_grid_tag
{
};

// This structure stores the layout requirements for the columns in a grid.
template<class Uniformity>
struct grid_column_requirements
{
};
// In nonuniform grids, each column has its own requirements.
template<>
struct grid_column_requirements<nonuniform_grid_tag>
{
    std::vector<layout_requirements> columns;
};
// In uniform grids, the columns all share the same requirements.
template<>
struct grid_column_requirements<uniform_grid_tag>
{
    size_t n_columns;
    layout_requirements requirements;
};

// Get the number of columns.
static size_t
get_column_count(grid_column_requirements<nonuniform_grid_tag> const& x)
{
    return x.columns.size();
}
static size_t
get_column_count(grid_column_requirements<uniform_grid_tag> const& x)
{
    return x.n_columns;
}

// Reset the column requirements.
static void
clear_requirements(grid_column_requirements<nonuniform_grid_tag>& x)
{
    x.columns.clear();
}
static void
clear_requirements(grid_column_requirements<uniform_grid_tag>& x)
{
    x.n_columns = 0;
    x.requirements = layout_requirements{0, 0, 0, 1};
}

// Add the requirements for a column.
static void
add_requirements(
    grid_column_requirements<nonuniform_grid_tag>& x,
    layout_requirements const& addition)
{
    x.columns.push_back(addition);
}
static void
add_requirements(
    grid_column_requirements<uniform_grid_tag>& x,
    layout_requirements const& addition)
{
    ++x.n_columns;
    fold_in_requirements(x.requirements, addition);
}

// Fold the second set of requirements into the first.
static void
fold_in_requirements(
    grid_column_requirements<nonuniform_grid_tag>& x,
    grid_column_requirements<nonuniform_grid_tag> const& y)
{
    size_t n_columns = get_column_count(y);
    if (get_column_count(x) < n_columns)
        x.columns.resize(n_columns, layout_requirements{0, 0, 0, 0});
    for (size_t i = 0; i != n_columns; ++i)
    {
        layout_requirements& xi = x.columns[i];
        layout_requirements const& yi = y.columns[i];
        fold_in_requirements(xi, yi);
        if (xi.growth_factor < yi.growth_factor)
            xi.growth_factor = yi.growth_factor;
    }
}
static void
fold_in_requirements(
    grid_column_requirements<uniform_grid_tag>& x,
    grid_column_requirements<uniform_grid_tag> const& y)
{
    if (x.n_columns < y.n_columns)
        x.n_columns = y.n_columns;
    fold_in_requirements(x.requirements, y.requirements);
}

// Get the requirements for the nth column.
inline layout_requirements const&
get_column_requirements(
    grid_column_requirements<nonuniform_grid_tag> const& x, size_t n)
{
    return x.columns[n];
}
inline layout_requirements const&
get_column_requirements(
    grid_column_requirements<uniform_grid_tag> const& x, size_t /*n*/)
{
    return x.requirements;
}

template<class Uniformity>
struct grid_row_container;

template<class Uniformity>
struct cached_grid_vertical_requirements
{
};

template<>
struct cached_grid_vertical_requirements<uniform_grid_tag>
{
    calculated_layout_requirements requirements;
    counter_type last_update = 1;
};

template<class Uniformity>
struct grid_data
{
    // the container that contains the whole grid
    widget_container* container = nullptr;

    // list of rows in the grid
    grid_row_container<Uniformity>* rows = nullptr;

    // spacing between columns
    layout_scalar column_spacing;

    // requirements for the columns
    grid_column_requirements<Uniformity> requirements;
    counter_type last_content_query = 0;

    // cached vertical requirements
    cached_grid_vertical_requirements<Uniformity> vertical_requirements_cache;

    // cached assignments
    std::vector<layout_scalar> assignments;
    counter_type last_assignments_update;
};

template<class Uniformity>
void
refresh_grid(
    layout_traversal<widget_container, widget>& traversal,
    grid_data<Uniformity>& data)
{
    if (traversal.is_refresh_pass)
    {
        // Reset the row list.
        data.rows = 0;
    }
}

template<class Uniformity>
void
refresh_grid_row(
    layout_traversal<widget_container, widget>& traversal,
    grid_data<Uniformity>& grid,
    grid_row_container<Uniformity>& row,
    layout const& layout_spec)
{
    // Add this row to the grid's list of rows.
    // It doesn't matter what order the list is in, and adding the row to
    // the front of the list is easier.
    if (traversal.is_refresh_pass)
    {
        row.next = grid.rows;
        grid.rows = &row;
        row.grid = &grid;
    }

    update_layout_cacher(traversal, row.cacher, layout_spec, FILL | UNPADDED);
}

struct scoped_grid_layout
{
    scoped_grid_layout()
    {
    }

    template<class Context>
    scoped_grid_layout(
        Context& ctx,
        layout const& layout_spec = default_layout,
        absolute_length const& column_spacing = absolute_length(0, PIXELS))
    {
        begin(ctx, layout_spec, column_spacing);
    }

    ~scoped_grid_layout()
    {
        end();
    }

    template<class Context>
    void
    begin(
        Context& ctx,
        layout const& layout_spec = default_layout,
        absolute_length const& column_spacing = absolute_length(0, PIXELS))
    {
        concrete_begin(
            get_layout_traversal(ctx),
            get_data_traversal(ctx),
            layout_spec,
            column_spacing);
    }

    void
    end()
    {
        container_.end();
    }

 private:
    void
    concrete_begin(
        layout_traversal<widget_container, widget>& traversal,
        data_traversal& data,
        layout const& layout_spec,
        absolute_length const& column_spacing);

    friend struct scoped_grid_row;

    scoped_layout_container container_;
    layout_traversal<widget_container, widget>* traversal_;
    data_traversal* data_traversal_;
    grid_data<nonuniform_grid_tag>* data_;
};

void
scoped_grid_layout::concrete_begin(
    layout_traversal<widget_container, widget>& traversal,
    data_traversal& data,
    layout const& layout_spec,
    absolute_length const& column_spacing)
{
    traversal_ = &traversal;
    data_traversal_ = &data;

    get_cached_data(data, &data_);
    refresh_grid(traversal, *data_);

    layout_container_widget<simple_layout_container<column_layout_logic>>*
        container;
    column_layout_logic* logic;
    get_simple_layout_container(
        traversal, data, &container, &logic, layout_spec);
    container_.begin(traversal, container);

    data_->container = container;

    layout_scalar resolved_spacing = as_layout_size(
        resolve_absolute_length(traversal, 0, column_spacing));
    detect_layout_change(traversal, &data_->column_spacing, resolved_spacing);
}

struct scoped_grid_row
{
    scoped_grid_row()
    {
    }
    scoped_grid_row(
        indie::context ctx,
        scoped_grid_layout const& g,
        layout const& layout_spec = default_layout)
    {
        begin(ctx, g, layout_spec);
    }
    ~scoped_grid_row()
    {
        end();
    }
    void
    begin(
        indie::context ctx,
        scoped_grid_layout const& g,
        layout const& layout_spec = default_layout);
    void
    end();

 private:
    scoped_layout_container container_;
};

template<class Uniformity>
struct grid_row_container : widget_container
{
    void
    render(render_event& event) override
    {
        auto const& region = get_assignment(this->cacher).region;
        SkRect bounds;
        bounds.fLeft = SkScalar(region.corner[0] + event.current_offset[0]);
        bounds.fTop = SkScalar(region.corner[1] + event.current_offset[1]);
        bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
        bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
        if (!event.canvas->quickReject(bounds))
        {
            auto original_offset = event.current_offset;
            event.current_offset += region.corner;
            indie::render_children(event, *this);
            event.current_offset = original_offset;
        }
    }

    void
    hit_test(
        hit_test_base& test, vector<2, double> const& point) const override
    {
        auto const& region = get_assignment(this->cacher).region;
        if (is_inside(region, vector<2, float>(point)))
        {
            auto local_point = point - vector<2, double>(region.corner);
            for (widget* node = this->widget_container::children; node;
                 node = node->next)
            {
                node->hit_test(test, local_point);
            }
        }
    }

    void
    process_input(event_context) override
    {
    }

    matrix<3, 3, double>
    transformation() const override
    {
        return parent->transformation();
    }

    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements() override;
    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override;
    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override;

    void
    record_content_change(
        layout_traversal<widget_container, widget>& traversal) override;
    void
    record_self_change(layout_traversal<widget_container, widget>& traversal);

    layout_cacher cacher;

    // cached requirements for cells within this row
    grid_column_requirements<Uniformity> requirements;
    counter_type last_content_query = 0;

    // reference to the data for the grid that this row belongs to
    grid_data<Uniformity>* grid = nullptr;

    // next row in this grid
    grid_row_container* next = nullptr;
};

// Update the requirements for a grid's columns by querying its contents.
template<class Uniformity>
void
update_grid_column_requirements(grid_data<Uniformity>& grid)
{
    // Only update if something in the grid has changed since the last
    // update.
    if (grid.last_content_query != grid.container->last_content_change)
    {
        // Clear the requirements for the grid and recompute them
        // by iterating through the rows and folding each row's
        // requirements into the main grid requirements.
        clear_requirements(grid.requirements);
        for (grid_row_container<Uniformity>* row = grid.rows; row;
             row = row->next)
        {
            // Again, only update if something in the row has
            // changed.
            if (row->last_content_query != row->last_content_change)
            {
                clear_requirements(row->requirements);
                for (widget* child = row->widget_container::children; child;
                     child = child->next)
                {
                    layout_requirements x
                        = child->get_horizontal_requirements();
                    add_requirements(row->requirements, x);
                }
                row->last_content_query = row->last_content_change;
            }
            fold_in_requirements(grid.requirements, row->requirements);
        }
        grid.last_content_query = grid.container->last_content_change;
    }
}

template<class Uniformity>
layout_scalar
get_required_width(grid_data<Uniformity> const& grid)
{
    size_t n_columns = get_column_count(grid.requirements);
    layout_scalar width = 0;
    for (size_t i = 0; i != n_columns; ++i)
        width += get_column_requirements(grid.requirements, i).size;
    if (n_columns > 0)
        width += grid.column_spacing * layout_scalar(n_columns - 1);
    return width;
}

template<class Uniformity>
float
get_total_growth(grid_data<Uniformity> const& grid)
{
    size_t n_columns = get_column_count(grid.requirements);
    float growth = 0;
    for (size_t i = 0; i != n_columns; ++i)
        growth += get_column_requirements(grid.requirements, i).growth_factor;
    return growth;
}

template<class Uniformity>
layout_requirements
grid_row_container<Uniformity>::get_horizontal_requirements()
{
    return cache_horizontal_layout_requirements(
        cacher, grid->container->last_content_change, [&] {
            update_grid_column_requirements(*grid);
            return calculated_layout_requirements{
                get_required_width(*grid), 0, 0};
        });
}

template<class Uniformity>
std::vector<layout_scalar> const&
calculate_column_assignments(
    grid_data<Uniformity>& grid, layout_scalar assigned_width)
{
    if (grid.last_assignments_update != grid.container->last_content_change)
    {
        update_grid_column_requirements(grid);
        size_t n_columns = get_column_count(grid.requirements);
        grid.assignments.resize(n_columns);
        layout_scalar required_width = get_required_width(grid);
        float total_growth = get_total_growth(grid);
        layout_scalar extra_width = assigned_width - required_width;
        for (size_t i = 0; i != n_columns; ++i)
        {
            layout_scalar width
                = get_column_requirements(grid.requirements, i).size;
            if (total_growth != 0)
            {
                float growth_factor
                    = get_column_requirements(grid.requirements, i)
                          .growth_factor;
                layout_scalar extra = round_to_layout_scalar(
                    (growth_factor / total_growth) * extra_width);
                extra_width -= extra;
                total_growth -= growth_factor;
                width += extra;
            }
            grid.assignments[i] = width;
        }
        grid.last_assignments_update = grid.container->last_content_change;
    }
    return grid.assignments;
}

calculated_layout_requirements
calculate_grid_row_vertical_requirements(
    grid_data<nonuniform_grid_tag>& grid,
    grid_row_container<nonuniform_grid_tag>& row,
    layout_scalar assigned_width)
{
    std::vector<layout_scalar> const& column_widths
        = calculate_column_assignments(grid, assigned_width);
    calculated_layout_requirements requirements{0, 0, 0};
    size_t column_index = 0;
    walk_layout_nodes(row.children, [&](layout_node_interface& node) {
        fold_in_requirements(
            requirements,
            node.get_vertical_requirements(column_widths[column_index]));
        ++column_index;
    });
    return requirements;
}

calculated_layout_requirements
calculate_grid_row_vertical_requirements(
    grid_data<uniform_grid_tag>& grid,
    grid_row_container<uniform_grid_tag>& /*row*/,
    layout_scalar assigned_width)
{
    named_block nb;
    auto& cache = grid.vertical_requirements_cache;
    if (cache.last_update != grid.container->last_content_change)
    {
        update_grid_column_requirements(grid);

        std::vector<layout_scalar> const& widths
            = calculate_column_assignments(grid, assigned_width);

        calculated_layout_requirements& grid_requirements = cache.requirements;
        grid_requirements = calculated_layout_requirements{0, 0, 0};
        for (grid_row_container<uniform_grid_tag>* row = grid.rows; row;
             row = row->next)
        {
            size_t column_index = 0;
            walk_layout_nodes(row->children, [&](layout_node_interface& node) {
                fold_in_requirements(
                    grid_requirements,
                    node.get_vertical_requirements(widths[column_index]));
                ++column_index;
            });
        }

        cache.last_update = grid.container->last_content_change;
    }
    return cache.requirements;
}

template<class Uniformity>
layout_requirements
grid_row_container<Uniformity>::get_vertical_requirements(
    layout_scalar assigned_width)
{
    return cache_vertical_layout_requirements(
        cacher, grid->container->last_content_change, assigned_width, [&] {
            return calculate_grid_row_vertical_requirements(
                *grid, *this, assigned_width);
        });
}

template<class Uniformity>
void
set_grid_row_relative_assignment(
    grid_data<Uniformity>& grid,
    widget* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    std::vector<layout_scalar> const& column_widths
        = calculate_column_assignments(grid, assigned_size[0]);
    size_t n = 0;
    layout_vector p = make_layout_vector(0, 0);
    for (widget* i = children; i; i = i->next, ++n)
    {
        layout_scalar this_width = column_widths[n];
        i->set_relative_assignment(relative_layout_assignment{
            layout_box(p, make_layout_vector(this_width, assigned_size[1])),
            assigned_baseline_y});
        p[0] += this_width + grid.column_spacing;
    }
}

template<class Uniformity>
void
grid_row_container<Uniformity>::set_relative_assignment(
    relative_layout_assignment const& assignment)
{
    update_relative_assignment(
        *this,
        cacher,
        grid->container->last_content_change,
        assignment,
        [&](auto const& resolved_assignment) {
            set_grid_row_relative_assignment(
                *grid,
                widget_container::children,
                resolved_assignment.region.size,
                resolved_assignment.baseline_y);
        });
}

template<class Uniformity>
void
grid_row_container<Uniformity>::record_content_change(
    layout_traversal<widget_container, widget>& traversal)
{
    if (this->last_content_change != traversal.refresh_counter)
    {
        this->last_content_change = traversal.refresh_counter;
        if (this->parent)
            this->parent->record_content_change(traversal);
        for (grid_row_container<Uniformity>* row = this->grid->rows; row;
             row = row->next)
        {
            row->record_self_change(traversal);
        }
    }
}

template<class Uniformity>
void
grid_row_container<Uniformity>::record_self_change(
    layout_traversal<widget_container, widget>& traversal)
{
    if (this->last_content_change != traversal.refresh_counter)
    {
        this->last_content_change = traversal.refresh_counter;
        if (this->parent)
            this->parent->record_content_change(traversal);
    }
}

void
scoped_grid_row::begin(
    indie::context, scoped_grid_layout const& grid, layout const& layout_spec)
{
    layout_traversal<widget_container, widget>& traversal = *grid.traversal_;

    grid_row_container<nonuniform_grid_tag>* row;
    if (get_cached_data(*grid.data_traversal_, &row))
        initialize(traversal, *row);

    refresh_grid_row(traversal, *grid.data_, *row, layout_spec);

    container_.begin(traversal, row);
}

void
scoped_grid_row::end()
{
    container_.end();
}

struct scrollable_view_data
{
    // This is the actual, unsmoothed scroll position.
    // If the user supplies external storage, then this is a copy of the value
    // stored there. Otherwise, this is the actual value.
    // (Either way, it's OK to read it, but writing should go through the
    // set_scroll_position function.)
    layout_vector scroll_position;

    // If this is true, the scroll_position has changed internally and needs
    // to be communicated to the external storage.
    // bool scroll_position_changed;

    // the smoothed version of the scroll position
    layout_vector smoothed_scroll_position;
    // for smoothing the scroll position
    value_smoother<layout_scalar> smoothers[2];

    // set by caller and copied here
    unsigned scrollable_axes, reserved_axes;

    // determined at usage site and needed by layout
    layout_scalar scrollbar_width, minimum_window_size, line_size;

    // determined by layout and stored here to communicate back to usage site
    bool hsb_on, vsb_on;
    layout_vector content_size, window_size;

    // // data for scrollbars
    scrollbar_data hsb_data, vsb_data;

    // // rendering data for junction
    // themed_rendering_data junction_rendering;

    // // layout container
    // scrollable_layout_container container;
};

struct scrollable_view : widget_container
{
    static float constexpr scrollbar_width = 30;

    void
    refresh(dataless_context ctx)
    {
        layout_box bg_area = get_assignment(this->cacher).region;
        bg_area.corner[0] += bg_area.size[0] - scrollbar_width;
        bg_area.size[0] = scrollbar_width;
        auto dsa = direct(scroll_amount);
        duplex<layout_scalar> position = dsa;
        refresh_scrollbar(
            ctx,
            scrollbar_parameters{
                &data.vsb_data,
                1,
                &position,
                bg_area,
                data.content_size[1],
                data.window_size[1],
                120,
                2400,
                internal_widget_ref{*this},
                0});
    }

    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements() override
    {
        auto child_requirements = cache_horizontal_layout_requirements(
            cacher, last_content_change, [&] {
                return logic->get_horizontal_requirements(children);
            });
        child_requirements.size += scrollbar_width;
        return child_requirements;
    }

    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override
    {
        return cache_vertical_layout_requirements(
            cacher, last_content_change, assigned_width, [&] {
                // TODO: Find a better way to determine the minimum window
                // size.
                return calculated_layout_requirements{100, 0, 0};
            });
    }

    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override
    {
        update_relative_assignment(
            *this,
            cacher,
            last_content_change,
            assignment,
            [&](auto const& resolved_assignment) {
                layout_vector available_size = resolved_assignment.region.size;

                calculated_layout_requirements x
                    = logic->get_horizontal_requirements(children);
                if (available_size[0] < x.size)
                {
                    data.hsb_on = true;
                    available_size[1] -= data.scrollbar_width;
                }
                else
                    data.hsb_on = false;

                calculated_layout_requirements y
                    = logic->get_vertical_requirements(
                        children, (std::max)(available_size[0], x.size));
                if (available_size[1] < y.size)
                {
                    data.vsb_on = true;
                    available_size[0] -= data.scrollbar_width;
                    if (!data.hsb_on && available_size[0] < x.size)
                    {
                        data.hsb_on = true;
                        available_size[1] -= data.scrollbar_width;
                    }
                }
                else
                    data.vsb_on = false;

                if ((data.reserved_axes & 1) != 0 && !data.hsb_on)
                    available_size[1] -= data.scrollbar_width;
                if ((data.reserved_axes & 2) != 0 && !data.vsb_on)
                    available_size[0] -= data.scrollbar_width;

                layout_scalar content_width
                    = (std::max)(available_size[0], x.size);

                y = logic->get_vertical_requirements(children, content_width);

                layout_scalar content_height
                    = (std::max)(available_size[1], y.size);

                layout_vector content_size
                    = make_layout_vector(content_width, content_height);

                // If the panel is scrolled all the way to the end, and the
                // content grows, scroll to show the new content.
                for (unsigned i = 0; i != 2; ++i)
                {
                    layout_scalar sp = data.smoothed_scroll_position[i];
                    if (sp != 0
                        && sp + data.window_size[i] >= data.content_size[i]
                        && sp + available_size[i] < data.content_size[i])
                    {
                        // TODO
                        // set_scroll_position_abruptly(
                        //     data, i, content_size[i] - available_size[i]);
                    }
                }

                data.content_size = content_size;
                data.window_size = available_size;

                // If the scroll position needs to be clamped because of
                // changes in content size, then do it abruptly, not smoothly.
                // TODO
                // for (unsigned i = 0; i != 2; ++i)
                // {
                //     layout_scalar original =
                //     data.smoothed_scroll_position[i]; layout_scalar clamped
                //         = clamp_scroll_position(data, i, original);
                //     if (clamped != original)
                //         set_scroll_position_abruptly(data, i, clamped);
                // }

                logic->set_relative_assignment(
                    children, content_size, content_height - y.descent);

                // this->assigned_size = resolved_assignment.region.size;
                // auto child_assignment = resolved_assignment;
                // child_assignment.region.size[0] -= scrollbar_width;
                // logic->set_relative_assignment(
                //     children,
                //     child_assignment.region.size,
                //     child_assignment.baseline_y);
            });
    }

    // layout_scalar
    // get_max_physical_position()
    // {
    //     return data.window_size[1]
    //            * (1 - data.window_size[1] / data.content_size[1]);
    // }

    // layout_scalar
    // get_max_logical_position()
    // {
    //     return data.content_size[1] - data.window_size[1];
    // }

    // layout_scalar
    // logical_position_to_physical(layout_scalar position)
    // {
    //     layout_scalar max_physical = get_max_physical_position();
    //     layout_scalar max_logical = get_max_logical_position();
    //     // return clamp(position * max_physical / max_logical, 0,
    //     // max_physical);
    //     return position * max_physical / max_logical;
    // }

    // layout_box
    // get_thumb_area()
    // {
    //     layout_box bg_area = get_assignment(this->cacher).region;
    //     bg_area.corner[0] += bg_area.size[0] - scrollbar_width;
    //     bg_area.size[0] = scrollbar_width;
    //     layout_box area = bg_area;
    //     area.size[1] = (std::max)(
    //         layout_scalar(0), // get_metrics(sb).minimum_thumb_length,
    //         data.window_size[1] * bg_area.size[1] / data.content_size[1]);
    //     area.corner[1]
    //         = bg_area.corner[1] +
    //         logical_position_to_physical(-scroll_amount);
    //     return area;
    // }

    void
    render(render_event& event) override
    {
        auto const& region = get_assignment(this->cacher).region;
        SkRect bounds;
        bounds.fLeft = SkScalar(region.corner[0] + event.current_offset[0]);
        bounds.fTop = SkScalar(region.corner[1] + event.current_offset[1]);
        bounds.fRight = bounds.fLeft + SkScalar(region.size[0]);
        bounds.fBottom = bounds.fTop + SkScalar(region.size[1]);
        if (!event.canvas->quickReject(bounds))
        {
            event.canvas->save();
            auto original_offset = event.current_offset;
            event.canvas->clipRect(bounds);
            event.canvas->translate(0, -scroll_amount);
            event.current_offset += region.corner;
            indie::render_children(event, *this);
            event.current_offset = original_offset;
            event.canvas->restore();
            // {
            //     SkPaint paint;
            //     paint.setColor(SK_ColorDKGRAY);
            //     auto thumb_area = get_thumb_area();
            //     SkRect box;
            //     box.fLeft
            //         = SkScalar(thumb_area.corner[0] +
            //         event.current_offset[0]);
            //     box.fTop
            //         = SkScalar(thumb_area.corner[1] +
            //         event.current_offset[1]);
            //     box.fRight = box.fLeft + SkScalar(thumb_area.size[0]);
            //     box.fBottom = box.fTop + SkScalar(thumb_area.size[1]);
            //     event.canvas->drawRect(box, paint);
            // }
            layout_box bg_area = get_assignment(this->cacher).region;
            bg_area.corner[0] += bg_area.size[0] - scrollbar_width;
            bg_area.size[0] = scrollbar_width;
            auto dsa = direct(scroll_amount);
            duplex<layout_scalar> position = dsa;
            render_scrollbar(
                event,
                scrollbar_parameters{
                    &data.vsb_data,
                    1,
                    &position,
                    bg_area,
                    data.content_size[1],
                    data.window_size[1],
                    120,
                    2400,
                    internal_widget_ref{*this},
                    0});
        }
    }

    void
    hit_test(
        hit_test_base& test, vector<2, double> const& point) const override
    {
        auto const& region = get_assignment(this->cacher).region;
        if (is_inside(region, vector<2, float>(point)))
        {
            if (test.type == indie::hit_test_type::WHEEL)
            {
                static_cast<indie::wheel_hit_test&>(test).result
                    = externalize(internal_element_ref{*this, 0});
            }
            auto local_point = point - vector<2, double>(region.corner);
            local_point[1] += scroll_amount;
            for (widget* node = this->widget_container::children; node;
                 node = node->next)
            {
                node->hit_test(test, local_point);
            }

            layout_box bg_area = get_assignment(this->cacher).region;
            bg_area.corner[0] += bg_area.size[0] - scrollbar_width;
            bg_area.size[0] = scrollbar_width;
            auto dsa = direct(scroll_amount);
            duplex<layout_scalar> position = dsa;
            hit_test_scrollbar(
                scrollbar_parameters{
                    &data.vsb_data,
                    1,
                    &position,
                    bg_area,
                    data.content_size[1],
                    data.window_size[1],
                    120,
                    2400,
                    internal_widget_ref{*this},
                    0},
                test,
                point);
        }
    }

    void
    process_input(event_context ctx) override
    {
        auto delta = detect_scroll(ctx, internal_element_ref{*this, 0});
        if (delta)
            scroll_amount -= (*delta)[1] * 120;

        layout_box bg_area = get_assignment(this->cacher).region;
        bg_area.corner[0] += bg_area.size[0] - scrollbar_width;
        bg_area.size[0] = scrollbar_width;
        auto dsa = direct(scroll_amount);
        duplex<layout_scalar> position = dsa;
        process_scrollbar_input(
            ctx,
            scrollbar_parameters{
                &data.vsb_data,
                1,
                &position,
                bg_area,
                data.content_size[1],
                data.window_size[1],
                120,
                2400,
                internal_widget_ref{*this},
                0});
    }

    matrix<3, 3, double>
    transformation() const override
    {
        return parent->transformation();
    }

    mutable scrollable_view_data data;

    column_layout_logic* logic;
    layout_cacher cacher;
    layout_vector assigned_size;

    mutable float scroll_amount = 0;
    value_smoother<float> smoother;
};

void
get_scrollable_view(
    indie::context ctx,
    std::shared_ptr<scrollable_view>** container,
    layout const& layout_spec)
{
    if (get_data(ctx, container))
        **container = std::make_shared<scrollable_view>();

    if (get_layout_traversal(ctx).is_refresh_pass)
    {
        (**container)->refresh(ctx);

        if (update_layout_cacher(
                get_layout_traversal(ctx),
                (**container)->cacher,
                layout_spec,
                FILL | UNPADDED))
        {
            // Since this container isn't active yet, it didn't get marked
            // as needing recalculation, so we need to do that manually
            // here.
            (**container)->last_content_change
                = get_layout_traversal(ctx).refresh_counter;
        }
    }
}

struct scoped_scrollable_view
{
    scoped_scrollable_view()
    {
    }
    scoped_scrollable_view(
        indie::context ctx, layout const& layout_spec = default_layout)
    {
        begin(ctx, layout_spec);
    }
    ~scoped_scrollable_view()
    {
        end();
    }
    void
    begin(indie::context ctx, layout const& layout_spec = default_layout)
    {
        std::shared_ptr<scrollable_view>* container;
        get_scrollable_view(ctx, &container, layout_spec);
        container_.begin(get_layout_traversal(ctx), container->get());
    }
    void
    end()
    {
        container_.end();
    }

 private:
    scoped_layout_container container_;
};
}} // namespace alia::indie

void
my_ui(indie::context ctx)
{
    std::cout << "sizeof(layout): " << sizeof(layout) << "\n";

    // static indie::layout_container_widget<column_layout> container;
    // static indie::simple_container_widget container;
    // indie::scoped_widget_container container_scope;
    // if (is_refresh_event(ctx))
    // {
    //     container_scope.begin(
    //         get<indie::traversal_tag>(ctx).widgets, &container);
    // }

    indie::scoped_column column(ctx, GROW | PADDED);

    auto show_text = get_state(ctx, false);

    do_box(ctx, SK_ColorLTGRAY, actions::toggle(show_text));
    do_spacer(ctx, height(100, PIXELS));

    {
        indie::scoped_grid_layout grid(ctx);
        {
            indie::scoped_grid_row row(ctx, grid);
            do_box(ctx, SK_ColorMAGENTA, actions::noop(), width(200, PIXELS));
            do_box(ctx, SK_ColorMAGENTA, actions::noop(), width(200, PIXELS));
        }
        {
            indie::scoped_grid_row row(ctx, grid);
            do_box(ctx, SK_ColorLTGRAY, actions::noop());
            do_box(ctx, SK_ColorLTGRAY, actions::noop());
        }
    }
    do_spacer(ctx, height(100, PIXELS));

    {
        scoped_scrollable_view scrollable(ctx, GROW);

        for (int outer = 0; outer != 10; ++outer)
        {
            // do_wrapped_text(
            //     ctx,
            //     value("Lorem ipsum dolor sit amet, consectetur adipiscing "
            //           "elit, "
            //           "sed do "
            //           "eiusmod tempor incididunt ut labore "
            //           "et dolore magna "
            //           "aliqua. Ut "
            //           "enim ad minim veniam, quis nostrud "
            //           "exercitation ullamco "
            //           "laboris "
            //           "nisi ut aliquip ex ea commodo "
            //           "consequat. Duis aute irure "
            //           "dolor "
            //           "in reprehenderit in voluptate velit esse cillum "
            //           "dolore "
            //           "eu "
            //           "fugiat "
            //           "nulla pariatur. Excepteur sint "
            //           "occaecat cupidatat non "
            //           "proident, "
            //           "sunt in culpa qui officia deserunt "
            //           "mollit anim id est "
            //           "laborum."),
            //     FILL);
        }

        // do_box(
        //     ctx,
        //     SK_ColorRED,
        //     actions::toggle(show_text),
        //     size(400, 400, PIXELS));
        // do_spacer(ctx, height(100, PIXELS));

        for (int outer = 0; outer != 10; ++outer)
        {
            indie::scoped_flow_layout flow(ctx, UNPADDED);

            for (int i = 0; i != 100; ++i)
            {
                if_(ctx, show_text, [&] {
                    // do_spacer(ctx, size(60, 40, PIXELS));
                    do_text(ctx, alia::printf(ctx, "text%i", i));
                    do_text(ctx, value("Könnten Sie mir das übersetzen?"));
                });

                {
                    indie::scoped_column col(ctx);

                    do_box(
                        ctx,
                        SK_ColorMAGENTA,
                        actions::noop(),
                        width(100, PIXELS));

                    // color::yiq<std::uint8_t> y1 =
                    // ::color::constant::blue_t{};
                    // color::yiq<std::uint8_t> y2 =
                    // ::color::constant::red_t{};
                    // color::yiq<std::uint8_t> yr =
                    // color::operation::mix(
                    //     y1,
                    //     std::max(
                    //         0.0,
                    //         std::min(
                    //             1.0,
                    //             std::fabs(std::sin(
                    //                 get_raw_animation_tick_count(ctx)
                    //                 / 1000.0)))),
                    //     y2);
                    // color::rgb<std::uint8_t> r(yr);
                    // color::rgb<std::uint8_t> r(y1);

                    do_box(ctx, SK_ColorLTGRAY, actions::noop());
                }

                {
                    indie::scoped_column col(ctx);

                    static SkColor clicky_color = SK_ColorRED;
                    // event_handler<indie::mouse_button_event>(
                    //     ctx, [&](auto, auto&) { clicky_color =
                    //     SK_ColorBLUE; });
                    do_box(ctx, clicky_color, actions::noop());

                    do_box(ctx, SK_ColorDKGRAY, actions::noop());

                    do_box(ctx, SK_ColorGRAY, actions::noop());
                }
            }
        }
    }
}
