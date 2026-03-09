/**
 * Alia font asset builder: reads a YAML manifest, resolves font paths/URLs
 * (downloads via libcurl with optional cache), generates a single MSDF atlas,
 * compresses with RLE, and emits alia_fonts.h / alia_fonts.cpp.
 *
 * Usage: alia_asset_builder <manifest.yaml> <output.h> <output.cpp>
 * [--cache-dir <dir>]
 */

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <msdfgen-ext.h>
#include <msdfgen.h>

#include "msdf-atlas-gen/BitmapAtlasStorage.h"
#include "msdf-atlas-gen/Charset.h"
#include "msdf-atlas-gen/FontGeometry.h"
#include "msdf-atlas-gen/ImmediateAtlasGenerator.h"
#include "msdf-atlas-gen/TightAtlasPacker.h"
#include "msdf-atlas-gen/glyph-generators.h"

#include <curl/curl.h>
#include <yaml-cpp/yaml.h>

using namespace msdf_atlas;

namespace fs = std::filesystem;

struct FontEntry
{
    std::string path; // resolved local path
    std::string id; // from manifest
    int weight = 400; // optional, default 400
    std::string slant; // optional, e.g. "normal", "italic"
};

static void
usage(const char* prog)
{
    fprintf(
        stderr,
        "Usage: %s <manifest.yaml> <output.h> <output.cpp>"
        " [--cache-dir <dir>]\n",
        prog);
}

// libcurl write callback to append to a file
static size_t
curl_write_file(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    FILE* fp = static_cast<FILE*>(userdata);
    size_t n = size * nmemb;
    return fp ? fwrite(ptr, 1, n, fp) : 0;
}

// Download `url` to `dest_path`.
// Returns 0 on success, non-zero on failure.
static int
download_url(const char* url, std::string const& dest_path)
{
    CURL* curl = curl_easy_init();
    if (!curl)
    {
        fprintf(stderr, "curl_easy_init failed\n");
        return 1;
    }
    FILE* fp = fopen(dest_path.c_str(), "wb");
    if (!fp)
    {
        fprintf(stderr, "Cannot open for write: %s\n", dest_path.c_str());
        curl_easy_cleanup(curl);
        return 1;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    if (res != CURLE_OK)
    {
        fprintf(stderr, "Download failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return 1;
    }
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);
    if (code < 200 || code >= 300)
    {
        fprintf(stderr, "HTTP %ld for %s\n", code, url);
        return 1;
    }
    return 0;
}

// Compress a single-channel buffer of the MSDF atlas.
//
// The image contains large regions of 0x00 and 0xff as well as smooth
// transitions between them, so we use RLE on those values (and those values
// only).
//
// The compressed buffer is a sequence of entries, where each entry is either:
// - a raw, single-byte value (`0x01` to `0xfe`)
// - a pair of bytes representing a run-length encoded value -
//   `(0x00 or 0xff, run_length_byte)`
//
static std::vector<uint8_t>
rle_compress_0x00_0xff_only(uint8_t const* data, size_t size)
{
    std::vector<uint8_t> out;
    size_t i = 0;
    while (i < size)
    {
        uint8_t v = data[i];
        if (v == 0x00 || v == 0xff)
        {
            size_t run = 1;
            while (i + run < size && data[i + run] == v && run < 255)
                ++run;
            out.push_back(v);
            out.push_back(static_cast<uint8_t>(run));
            i += run;
        }
        else
        {
            out.push_back(v);
            i += 1;
        }
    }
    return out;
}

int
main(int argc, char const* const* argv)
{
    char const* manifest_path = nullptr;
    char const* out_h = nullptr;
    char const* out_cpp = nullptr;
    char const* cache_dir = nullptr;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--cache-dir") == 0 && i + 1 < argc)
        {
            cache_dir = argv[++i];
            continue;
        }
        if (!manifest_path)
            manifest_path = argv[i];
        else if (!out_h)
            out_h = argv[i];
        else if (!out_cpp)
            out_cpp = argv[i];
    }
    if (!manifest_path || !out_h || !out_cpp)
    {
        usage(argv[0]);
        return 1;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK)
    {
        fprintf(stderr, "curl_global_init failed\n");
        return 1;
    }

    // Parse YAML manifest
    YAML::Node doc;
    try
    {
        doc = YAML::LoadFile(manifest_path);
    }
    catch (YAML::Exception const& e)
    {
        fprintf(stderr, "Invalid manifest %s: %s\n", manifest_path, e.what());
        return 1;
    }
    YAML::Node fonts_node = doc["fonts"];
    if (!fonts_node || !fonts_node.IsSequence())
    {
        fprintf(stderr, "Manifest must contain a 'fonts' sequence\n");
        return 1;
    }

    fs::path manifest_dir = fs::path(manifest_path).parent_path();
    std::vector<FontEntry> font_entries;

    for (size_t i = 0; i < fonts_node.size(); ++i)
    {
        YAML::Node entry = fonts_node[i];
        if (!entry["id"])
        {
            fprintf(stderr, "Font entry %zu: missing 'id'\n", i);
            return 1;
        }
        std::string id = entry["id"].as<std::string>();
        bool has_path = entry["path"].IsDefined();
        bool has_url = entry["url"].IsDefined();
        if (has_path == has_url)
        {
            fprintf(
                stderr,
                "Font '%s': exactly one of 'path' or 'url' required\n",
                id.c_str());
            return 1;
        }

        FontEntry fe;
        fe.id = id;
        if (entry["weight"])
            fe.weight = entry["weight"].as<int>();
        if (entry["slant"])
            fe.slant = entry["slant"].as<std::string>();

        if (has_path)
        {
            std::string path_str = entry["path"].as<std::string>();
            fs::path p(path_str);
            if (!p.is_absolute())
                p = manifest_dir / p;
            fe.path = p.lexically_normal().string();
            if (!fs::is_regular_file(fe.path))
            {
                fprintf(
                    stderr,
                    "Font '%s': file not found: %s\n",
                    id.c_str(),
                    fe.path.c_str());
                return 1;
            }
        }
        else
        {
            std::string url = entry["url"].as<std::string>();
            if (cache_dir)
            {
                fs::path cache_path = fs::path(cache_dir) / (id + ".ttf");
                if (!fs::is_regular_file(cache_path.string()))
                {
                    fs::create_directories(cache_dir);
                    if (download_url(url.c_str(), cache_path.string()) != 0)
                    {
                        fprintf(
                            stderr,
                            "Failed to download font '%s'\n",
                            id.c_str());
                        return 1;
                    }
                }
                fe.path = cache_path.lexically_normal().string();
            }
            else
            {
                // Temp file (no cache)
                fs::path tmp
                    = fs::temp_directory_path() / ("alia_font_" + id + ".ttf");
                if (download_url(url.c_str(), tmp.string()) != 0)
                {
                    fprintf(
                        stderr, "Failed to download font '%s'\n", id.c_str());
                    return 1;
                }
                fe.path = tmp.lexically_normal().string();
            }
        }
        font_entries.push_back(std::move(fe));
    }

    if (font_entries.empty())
    {
        fprintf(stderr, "No fonts in manifest\n");
        return 1;
    }

    msdfgen::FreetypeHandle* ft = msdfgen::initializeFreetype();
    if (!ft)
    {
        fprintf(stderr, "Failed to initialize FreeType\n");
        return 1;
    }

    std::vector<GlyphGeometry> glyphs;
    std::vector<FontGeometry> fonts;
    double font_scale = 1.0;

    for (size_t f = 0; f < font_entries.size(); ++f)
    {
        char const* path = font_entries[f].path.c_str();
        msdfgen::FontHandle* font = msdfgen::loadFont(ft, path);
        if (!font)
        {
            fprintf(stderr, "Failed to load font: %s\n", path);
            msdfgen::deinitializeFreetype(ft);
            return 1;
        }
        FontGeometry font_geometry(&glyphs);
        int loaded = font_geometry.loadCharset(
            font, font_scale, Charset::ASCII, true, true);
        if (loaded <= 0)
        {
            fprintf(stderr, "No glyphs loaded from %s\n", path);
            msdfgen::destroyFont(font);
            msdfgen::deinitializeFreetype(ft);
            return 1;
        }
        fprintf(stderr, "Loaded %d glyphs from %s\n", loaded, path);
        fonts.push_back(std::move(font_geometry));
        msdfgen::destroyFont(font);
    }

    if (glyphs.empty())
    {
        fprintf(stderr, "No glyphs loaded\n");
        msdfgen::deinitializeFreetype(ft);
        return 1;
    }

    // Pack atlas
    TightAtlasPacker packer;
    packer.setDimensionsConstraint(
        DimensionsConstraint::MULTIPLE_OF_FOUR_SQUARE);
    packer.setSpacing(0);
    packer.setScale(48.0);
    packer.setPixelRange(msdfgen::Range(2.0));
    packer.setMiterLimit(1.0);
    packer.setOriginPixelAlignment(false, true);

    if (int remaining
        = packer.pack(glyphs.data(), static_cast<int>(glyphs.size())))
    {
        fprintf(stderr, "Failed to pack glyphs (remaining=%d)\n", remaining);
        msdfgen::deinitializeFreetype(ft);
        return 1;
    }

    int atlas_width = 0, atlas_height = 0;
    packer.getDimensions(atlas_width, atlas_height);
    double em_size = packer.getScale();
    msdfgen::Range px_range = packer.getPixelRange();

    // Edge coloring for MSDF
    unsigned long long seed = 0;
    for (GlyphGeometry& g : glyphs)
    {
        seed *= 6364136223846793005ull;
        seed += 1442695040888963407ull;
        g.edgeColoring(msdfgen::edgeColoringInkTrap, 3.0, seed);
    }

    // Generate atlas bitmap (float RGB)
    using Storage = BitmapAtlasStorage<float, 3>;
    ImmediateAtlasGenerator<float, 3, msdfGenerator, Storage> generator(
        atlas_width, atlas_height);
    GeneratorAttributes gen_attrs;
    gen_attrs.config.overlapSupport = false;
    gen_attrs.scanlinePass = true;

    generator.setAttributes(gen_attrs);
    generator.setThreadCount(1);
    generator.generate(glyphs.data(), static_cast<int>(glyphs.size()));

    msdfgen::BitmapConstRef<float, 3> bitmap_ref
        = static_cast<msdfgen::BitmapConstRef<float, 3>>(
            generator.atlasStorage());
    int w = bitmap_ref.width;
    int h = bitmap_ref.height;
    if (w <= 0 || h <= 0 || !bitmap_ref.pixels)
    {
        fprintf(stderr, "Invalid atlas bitmap\n");
        msdfgen::deinitializeFreetype(ft);
        return 1;
    }

    // Convert float [0,1] to uint8_t, split into R/G/B planes, RLE compress
    // each
    size_t raw_size = static_cast<size_t>(w) * h * 3;
    size_t plane_size = static_cast<size_t>(w) * h;
    std::vector<uint8_t> raw(raw_size);
    float const* src = bitmap_ref.pixels;
    for (size_t i = 0; i < raw_size; ++i)
    {
        float v = src[i];
        if (v < 0.f)
            v = 0.f;
        if (v > 1.f)
            v = 1.f;
        raw[i] = static_cast<uint8_t>(v * 255.f + 0.5f);
    }
    std::vector<uint8_t> r_plane(plane_size), g_plane(plane_size),
        b_plane(plane_size);
    for (size_t i = 0; i < plane_size; ++i)
    {
        r_plane[i] = raw[i * 3 + 0];
        g_plane[i] = raw[i * 3 + 1];
        b_plane[i] = raw[i * 3 + 2];
    }
    std::vector<uint8_t> rle_r
        = rle_compress_0x00_0xff_only(r_plane.data(), plane_size);
    std::vector<uint8_t> rle_g
        = rle_compress_0x00_0xff_only(g_plane.data(), plane_size);
    std::vector<uint8_t> rle_b
        = rle_compress_0x00_0xff_only(b_plane.data(), plane_size);

    msdfgen::deinitializeFreetype(ft);

    // --- Emit alia_fonts.h ---
    {
        std::ofstream f(out_h);
        if (!f)
        {
            fprintf(stderr, "Cannot write %s\n", out_h);
            return 1;
        }
        f << R"(#pragma once

#include <cstddef>
#include <cstdint>

#include <alia/text_engines/msdf/msdf.hpp>

namespace alia {

extern std::uint8_t const alia_atlas_rle_r[];
extern std::size_t const alia_atlas_rle_r_size;
extern std::uint8_t const alia_atlas_rle_g[];
extern std::size_t const alia_atlas_rle_g_size;
extern std::uint8_t const alia_atlas_rle_b[];
extern std::size_t const alia_atlas_rle_b_size;

inline int const alia_atlas_width = )"
          << w << R"(;
inline int const alia_atlas_height = )"
          << h << R"(;
inline std::size_t const alia_atlas_decompressed_size = )"
          << raw_size << R"(;

)";

        for (size_t font_idx = 0; font_idx < fonts.size(); ++font_idx)
        {
            f << "extern alia::msdf_glyph const alia_font_" << font_idx
              << "_glyphs[];\n";
            f << "extern std::size_t const alia_font_" << font_idx
              << "_glyph_count;\n";
            if (!fonts[font_idx].getKerning().empty())
            {
                f << "extern alia::msdf_kerning_pair const alia_font_"
                  << font_idx << "_kerning_pairs[];\n";
            }
            f << "extern std::size_t const alia_font_" << font_idx
              << "_kerning_pair_count;\n";
            // Per-font metadata from manifest
            f << "extern char const alia_font_" << font_idx << "_id[];\n";
            f << "inline int const alia_font_" << font_idx
              << "_weight = " << font_entries[font_idx].weight << ";\n";
            f << "extern char const alia_font_" << font_idx << "_slant[];\n";
        }

        float const dist_range
            = static_cast<float>(px_range.upper - px_range.lower);
        float const em_sz = static_cast<float>(em_size);

        f << R"(
inline std::size_t const alia_font_count = )"
          << fonts.size() << R"(;

inline msdf_font_description const& alia_font_description(std::size_t index) {
    static msdf_font_description const descs[] = {
)";
        for (size_t font_idx = 0; font_idx < fonts.size(); ++font_idx)
        {
            FontGeometry const& fg = fonts[font_idx];
            msdfgen::FontMetrics const& m = fg.getMetrics();
            f << "        { {" << std::showpoint
              << static_cast<float>(m.emSize) << "f, "
              << static_cast<float>(m.lineHeight) << "f, "
              << static_cast<float>(m.ascenderY) << "f, "
              << static_cast<float>(m.descenderY) << "f, "
              << static_cast<float>(m.underlineY) << "f, "
              << static_cast<float>(m.underlineThickness) << "f}, {"
              << dist_range << "f, 0.f, " << em_sz << "f, "
              << static_cast<float>(w) << "f, " << static_cast<float>(h)
              << "f}, alia_font_" << font_idx << "_glyphs, alia_font_"
              << font_idx << "_glyph_count, "
              << (fonts[font_idx].getKerning().empty()
                      ? "nullptr"
                      : "alia_font_" + std::to_string(font_idx)
                            + "_kerning_pairs")
              << ", alia_font_" << font_idx << "_kerning_pair_count },\n";
        }
        f << "    };\n    return descs[index];\n}\n\n} // namespace alia\n";
    }

    // --- Emit alia_fonts.cpp (glyph/kerning arrays and RLE data) ---
    {
        std::ofstream f(out_cpp);
        if (!f)
        {
            fprintf(stderr, "Cannot write %s\n", out_cpp);
            return 1;
        }
        f << "#include \"alia_fonts.h\"\n\nnamespace alia {\n\n";

        for (size_t font_idx = 0; font_idx < fonts.size(); ++font_idx)
        {
            FontGeometry const& fg = fonts[font_idx];
            auto range = fg.getGlyphs();

            f << "alia::msdf_glyph const alia_font_" << font_idx
              << "_glyphs[] = {\n";
            f << std::showpoint;
            for (const GlyphGeometry* it = range.begin(); it != range.end();
                 ++it)
            {
                GlyphGeometry const& g = *it;
                double pl, pb, pr, pt, al, ab, ar, at;
                g.getQuadPlaneBounds(pl, pb, pr, pt);
                g.getQuadAtlasBounds(al, ab, ar, at);
                uint32_t cp = static_cast<uint32_t>(g.getCodepoint());
                float adv = static_cast<float>(g.getAdvance());
                int vis = g.isWhitespace() ? 0 : 1;
                f << "    { " << cp << "u, " << adv << "f, " << vis << ", "
                  << static_cast<float>(pl) << "f, " << static_cast<float>(pb)
                  << "f, " << static_cast<float>(pr) << "f, "
                  << static_cast<float>(pt) << "f, " << static_cast<float>(al)
                  << "f, " << static_cast<float>(ab) << "f, "
                  << static_cast<float>(ar) << "f, " << static_cast<float>(at)
                  << "f },\n";
            }
            f << "};\n";
            f << "std::size_t const alia_font_" << font_idx
              << "_glyph_count = " << (range.end() - range.begin()) << ";\n\n";

            // Kerning: iterate getKerning() and output (unicode, unicode,
            // adjustment)
            auto const& kerning = fg.getKerning();
            std::vector<std::string> kp_lines;
            for (auto const& kv : kerning)
            {
                msdfgen::GlyphIndex i1
                    = static_cast<msdfgen::GlyphIndex>(kv.first.first);
                msdfgen::GlyphIndex i2
                    = static_cast<msdfgen::GlyphIndex>(kv.first.second);
                GlyphGeometry const* g1 = fg.getGlyph(i1);
                GlyphGeometry const* g2 = fg.getGlyph(i2);
                if (g1 && g2)
                {
                    uint32_t u1 = static_cast<uint32_t>(g1->getCodepoint());
                    uint32_t u2 = static_cast<uint32_t>(g2->getCodepoint());
                    float adj = static_cast<float>(kv.second);
                    kp_lines.push_back(
                        "    { " + std::to_string(u1) + "u, "
                        + std::to_string(u2) + "u, " + std::to_string(adj)
                        + "f }");
                }
            }
            if (!kp_lines.empty())
            {
                f << "alia::msdf_kerning_pair const alia_font_" << font_idx
                  << "_kerning_pairs[] = {\n";
                for (size_t i = 0; i < kp_lines.size(); ++i)
                {
                    f << kp_lines[i];
                    f << (i + 1 < kp_lines.size() ? ",\n" : "\n");
                }
                f << "};\n";
            }
            f << "std::size_t const alia_font_" << font_idx
              << "_kerning_pair_count = " << kp_lines.size() << ";\n\n";

            // Per-font metadata from manifest
            std::string id_escaped;
            for (char c : font_entries[font_idx].id)
            {
                if (c == '\\')
                    id_escaped += "\\\\";
                else if (c == '"')
                    id_escaped += "\\\"";
                else
                    id_escaped += c;
            }
            std::string slant_escaped;
            for (char c : font_entries[font_idx].slant)
            {
                if (c == '\\')
                    slant_escaped += "\\\\";
                else if (c == '"')
                    slant_escaped += "\\\"";
                else
                    slant_escaped += c;
            }
            f << "char const alia_font_" << font_idx << "_id[] = \""
              << id_escaped << "\";\n";
            f << "char const alia_font_" << font_idx << "_slant[] = \""
              << slant_escaped << "\";\n\n";
        }

        f << "std::uint8_t const alia_atlas_rle_r[] = {\n";
        f << std::hex << std::setfill('0');
        for (size_t i = 0; i < rle_r.size(); i += 12)
        {
            f << "    ";
            for (size_t j = i; j < rle_r.size() && j < i + 12; ++j)
                f << "0x" << std::setw(2) << static_cast<int>(rle_r[j])
                  << (j + 1 < rle_r.size() ? ", " : "");
            f << "\n";
        }
        f << std::dec << std::setfill(' ');
        f << "};\n";
        f << "std::size_t const alia_atlas_rle_r_size = " << rle_r.size()
          << ";\n\n";
        f << "std::uint8_t const alia_atlas_rle_g[] = {\n";
        f << std::hex << std::setfill('0');
        for (size_t i = 0; i < rle_g.size(); i += 12)
        {
            f << "    ";
            for (size_t j = i; j < rle_g.size() && j < i + 12; ++j)
                f << "0x" << std::setw(2) << static_cast<int>(rle_g[j])
                  << (j + 1 < rle_g.size() ? ", " : "");
            f << "\n";
        }
        f << std::dec << std::setfill(' ');
        f << "};\n";
        f << "std::size_t const alia_atlas_rle_g_size = " << rle_g.size()
          << ";\n\n";
        f << "std::uint8_t const alia_atlas_rle_b[] = {\n";
        f << std::hex << std::setfill('0');
        for (size_t i = 0; i < rle_b.size(); i += 12)
        {
            f << "    ";
            for (size_t j = i; j < rle_b.size() && j < i + 12; ++j)
                f << "0x" << std::setw(2) << static_cast<int>(rle_b[j])
                  << (j + 1 < rle_b.size() ? ", " : "");
            f << "\n";
        }
        f << std::dec << std::setfill(' ');
        f << "};\n";
        f << "std::size_t const alia_atlas_rle_b_size = " << rle_b.size()
          << ";\n\n} // namespace alia\n";
    }

    fprintf(
        stderr,
        "Wrote %s and %s (atlas %dx%d, RLE R=%zu G=%zu B=%zu total=%zu "
        "bytes)\n",
        out_h,
        out_cpp,
        w,
        h,
        rle_r.size(),
        rle_g.size(),
        rle_b.size(),
        rle_r.size() + rle_g.size() + rle_b.size());
    curl_global_cleanup();
    return 0;
}
