#include <alia/ui/text/shaping.hpp>

#include <alia/ui/text/fonts.hpp>

#ifdef _WIN32
#pragma warning(push, 0)
#endif

#include <include/core/SkFontMetrics.h>
#include <modules/skshaper/include/SkShaper.h>
#include <src/base/SkUTF.h>

namespace alia {

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

    SkScalar
    ascent() const
    {
        return fMaxRunAscent;
    }
    SkScalar
    descent() const
    {
        return fMaxRunDescent;
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
    std::unique_ptr<SkShaper> shaper = SkShaper::Make(get_skia_font_manager());
    result.width = 0;
    float height = font.getSpacing();
    RunHandler runHandler(utf8Text, textByteLen);
    if (textByteLen)
    {
        result.glyphBounds.resize(textByteLen);
        for (SkRect& c : result.glyphBounds)
        {
            c = SkRect{
                -std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max()};
        }
        runHandler.setRunCallback(
            set_character_bounds, result.glyphBounds.data());

        std::unique_ptr<SkShaper::BiDiRunIterator> bidi(
            SkShaper::MakeBiDiRunIterator(utf8Text, textByteLen, 0xfe));
        if (!bidi)
        {
            return result;
        }
        std::unique_ptr<SkShaper::LanguageRunIterator> language(
            SkShaper::MakeStdLanguageRunIterator(utf8Text, textByteLen));
        if (!language)
        {
            return result;
        }
        SkFourByteTag undeterminedScript
            = SkSetFourByteTag('Z', 'y', 'y', 'y');
        std::unique_ptr<SkShaper::ScriptRunIterator> script(
            SkShaper::MakeScriptRunIterator(
                utf8Text, textByteLen, undeterminedScript));
        if (!script)
        {
            return result;
        }
        std::unique_ptr<SkShaper::FontRunIterator> fri(
            SkShaper::MakeFontMgrRunIterator(
                utf8Text,
                textByteLen,
                font,
                get_skia_font_manager(),
                "Arial",
                SkFontStyle::Bold(),
                &*language));
        if (!fri)
        {
            return result;
        }

        shaper->shape(
            utf8Text,
            textByteLen,
            *fri,
            *bidi,
            *script,
            *language,
            width,
            &runHandler);
        if (runHandler.lineEndOffsets().size() > 1)
        {
            result.lineBreakOffsets = runHandler.lineEndOffsets();
            SkASSERT(result.lineBreakOffsets.size() > 0);
            result.lineBreakOffsets.pop_back();
        }
        height = std::max(height, runHandler.endPoint().y());
        result.width = runHandler.finalPosition().x();
        result.ascent = -runHandler.ascent();
        result.descent = runHandler.descent();
        // std::cout << "result.width = " << result.width << std::endl;
        result.blob = runHandler.makeBlob();
    }
    result.glyphBounds.push_back(runHandler.finalRect(font));
    result.verticalAdvance = (int) ceilf(height);
    return result;
}

} // namespace alia
