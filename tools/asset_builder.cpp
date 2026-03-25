/**
 * Alia font asset builder: reads a YAML manifest, resolves font paths/URLs
 * (downloads via libcurl with optional cache), generates a single MSDF atlas,
 * compresses with RLE, and emits alia_fonts.h / alia_fonts.cpp.
 *
 * Text fonts load printable ASCII. Icon fonts (optional `icons` plus
 * `codepoints_path` or `codepoints_url`) bake only listed glyphs from a Google
 * `.codepoints` file and emit Unicode constants per icon.
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
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

static bool
is_ident_start(unsigned char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool
is_ident_char(unsigned char c)
{
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

struct FontEntry
{
    std::string path; // resolved local path (TTF)
    std::string id; // from manifest; must be a valid C identifier
    /// If empty, load printable ASCII; otherwise icon font with these
    /// (name, codepoint) pairs in manifest order.
    std::vector<std::pair<std::string, uint32_t>> icon_codepoints;
    /// Optional variable-font design coordinates by OpenType axis tag.
    std::vector<std::pair<std::string, double>> variation_axes;
};

static int
apply_variation_axes(
    FontEntry const& entry,
    msdfgen::FreetypeHandle* ft,
    msdfgen::FontHandle* font)
{
    if (entry.variation_axes.empty())
        return 0;

    std::vector<msdfgen::FontVariationAxis> available_axes;
    if (!msdfgen::listFontVariationAxes(available_axes, ft, font))
    {
        fprintf(
            stderr,
            "Font '%s': variable axes were requested, but this font does not "
            "expose variation axes\n",
            entry.id.c_str());
        return 1;
    }

    auto canonicalize = [](std::string s) {
        std::string out;
        out.reserve(s.size());
        for (char c : s)
        {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                || (c >= '0' && c <= '9'))
            {
                if (c >= 'A' && c <= 'Z')
                    out.push_back(char(c - 'A' + 'a'));
                else
                    out.push_back(c);
            }
        }
        return out;
    };

    auto normalized_axis_alias = [&](std::string const& requested) {
        std::string norm = canonicalize(requested);
        if (norm == "wght")
            return std::string("weight");
        if (norm == "wdth")
            return std::string("width");
        if (norm == "opsz")
            return std::string("opticalsize");
        if (norm == "slnt")
            return std::string("slant");
        if (norm == "ital")
            return std::string("italic");
        return norm;
    };

    std::unordered_map<std::string, msdfgen::FontVariationAxis const*> axis_lookup;
    for (auto const& axis : available_axes)
    {
        std::string key = canonicalize(axis.name ? axis.name : "");
        axis_lookup[key] = &axis;
    }

    for (auto const& axis_req : entry.variation_axes)
    {
        std::string key = normalized_axis_alias(axis_req.first);
        auto it = axis_lookup.find(key);
        if (it == axis_lookup.end())
        {
            fprintf(
                stderr,
                "Font '%s': unknown variation axis '%s' (available:",
                entry.id.c_str(),
                axis_req.first.c_str());
            for (auto const& axis : available_axes)
                fprintf(stderr, " %s", axis.name ? axis.name : "<null>");
            fprintf(stderr, " )\n");
            return 1;
        }

        auto const& axis = *it->second;
        double requested = axis_req.second;
        double min_v = axis.minValue;
        double max_v = axis.maxValue;
        double applied = requested;
        if (applied < min_v)
            applied = min_v;
        if (applied > max_v)
            applied = max_v;

        if (applied != requested)
        {
            fprintf(
                stderr,
                "Font '%s': axis '%s' value %.4f out of range [%.4f, %.4f], "
                "clamped to %.4f\n",
                entry.id.c_str(),
                axis_req.first.c_str(),
                requested,
                min_v,
                max_v,
                applied);
        }

        char const* axis_name = it->second->name;
        if (!axis_name
            || !msdfgen::setFontVariationAxis(ft, font, axis_name, applied))
        {
            fprintf(
                stderr,
                "Font '%s': failed to set variation axis '%s' to %.4f\n",
                entry.id.c_str(),
                axis_req.first.c_str(),
                applied);
            return 1;
        }
    }

    return 0;
}

static bool
is_valid_c_identifier(std::string const& s)
{
    if (s.empty())
        return false;
    if (!is_ident_start(static_cast<unsigned char>(s[0])))
        return false;
    for (size_t i = 1; i < s.size(); ++i)
    {
        if (!is_ident_char(static_cast<unsigned char>(s[i])))
            return false;
    }
    return true;
}

/// Suffix for `alia_font_<Id>_icon_<suffix>` when the Google icon name is not
/// a valid C identifier (e.g. starts with a digit). Uses `cp_` prefix so we
/// do not collide with a distinct icon literally named `u10k` vs `10k`.
static std::string
icon_name_c_suffix(std::string const& icon_name)
{
    if (icon_name.empty())
        return "invalid_empty";
    unsigned char c0 = static_cast<unsigned char>(icon_name[0]);
    if (is_ident_start(c0))
        return icon_name;
    return std::string("cp_") + icon_name;
}

static bool
parse_hex_codepoint(std::string const& hex_in, uint32_t& out)
{
    std::string hex = hex_in;
    while (!hex.empty() && (hex.front() == ' ' || hex.front() == '\t'))
        hex.erase(hex.begin());
    while (!hex.empty() && (hex.back() == ' ' || hex.back() == '\t'))
        hex.pop_back();
    if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X'))
        hex = hex.substr(2);
    if (hex.empty())
        return false;
    char* end = nullptr;
    unsigned long v = std::strtoul(hex.c_str(), &end, 16);
    if (end != hex.c_str() + hex.size())
        return false;
    out = static_cast<uint32_t>(v);
    return true;
}

// Google `.codepoints` format: one line per glyph: `name hex` (hex without 0x).
static int
load_codepoints_map(
    std::string const& path, std::unordered_map<std::string, uint32_t>& out)
{
    std::ifstream in(path);
    if (!in)
    {
        fprintf(stderr, "Cannot read codepoints file: %s\n", path.c_str());
        return 1;
    }
    std::string line;
    size_t line_no = 0;
    while (std::getline(in, line))
    {
        ++line_no;
        // Trim
        size_t a = 0;
        while (a < line.size()
               && (line[a] == ' ' || line[a] == '\t' || line[a] == '\r'))
            ++a;
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t'
                                  || line.back() == '\r'))
            line.pop_back();
        if (a >= line.size())
            continue;
        line = line.substr(a);
        if (line[0] == '#')
            continue;
        std::istringstream iss(line);
        std::string name;
        std::string hex;
        if (!(iss >> name >> hex))
        {
            fprintf(
                stderr,
                "%s:%zu: expected 'name hex'\n",
                path.c_str(),
                line_no);
            return 1;
        }
        uint32_t cp = 0;
        if (!parse_hex_codepoint(hex, cp))
        {
            fprintf(
                stderr,
                "%s:%zu: bad hex for '%s'\n",
                path.c_str(),
                line_no,
                name.c_str());
            return 1;
        }
        out[std::move(name)] = cp;
    }
    return 0;
}

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
    std::unordered_set<std::string> seen_ids;

    for (size_t i = 0; i < fonts_node.size(); ++i)
    {
        YAML::Node entry = fonts_node[i];
        if (!entry["id"])
        {
            fprintf(stderr, "Font entry %zu: missing 'id'\n", i);
            return 1;
        }
        std::string id = entry["id"].as<std::string>();
        if (!is_valid_c_identifier(id))
        {
            fprintf(
                stderr,
                "Font '%s': 'id' must be a valid C identifier "
                "([A-Za-z_][A-Za-z0-9_]*)\n",
                id.c_str());
            return 1;
        }
        if (!seen_ids.insert(id).second)
        {
            fprintf(stderr, "Duplicate font id: %s\n", id.c_str());
            return 1;
        }

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

        YAML::Node icons_node = entry["icons"];
        YAML::Node axes_node = entry["axes"];
        bool has_icons_list = icons_node && icons_node.IsSequence();
        bool has_icons = has_icons_list && icons_node.size() > 0;
        bool has_codepoints_path = entry["codepoints_path"].IsDefined();
        bool has_codepoints_url = entry["codepoints_url"].IsDefined();
        if (has_codepoints_path == has_codepoints_url)
        {
            if (has_icons)
            {
                fprintf(
                    stderr,
                    "Font '%s': exactly one of 'codepoints_path' or "
                    "'codepoints_url' required when 'icons' is set\n",
                    id.c_str());
                return 1;
            }
        }
        else if (!has_icons)
        {
            fprintf(
                stderr,
                "Font '%s': 'icons' required when codepoints_path/url is set\n",
                id.c_str());
            return 1;
        }
        if (has_icons_list && icons_node.size() == 0)
        {
            fprintf(
                stderr,
                "Font '%s': 'icons' must be non-empty when present\n",
                id.c_str());
            return 1;
        }

        FontEntry fe;
        fe.id = id;

        if (axes_node)
        {
            if (!axes_node.IsMap())
            {
                fprintf(
                    stderr,
                    "Font '%s': 'axes' must be a mapping of axis_tag: value\n",
                    id.c_str());
                return 1;
            }
            for (auto const& axis_item : axes_node)
            {
                if (!axis_item.first.IsScalar() || !axis_item.second.IsScalar())
                {
                    fprintf(
                        stderr,
                        "Font '%s': axis entries must be scalar tag/value pairs\n",
                        id.c_str());
                    return 1;
                }
                std::string axis_tag = axis_item.first.as<std::string>();
                double axis_value = 0;
                try
                {
                    axis_value = axis_item.second.as<double>();
                }
                catch (YAML::Exception const&)
                {
                    fprintf(
                        stderr,
                        "Font '%s': axis '%s' value must be numeric\n",
                        id.c_str(),
                        axis_tag.c_str());
                    return 1;
                }
                fe.variation_axes.push_back({std::move(axis_tag), axis_value});
            }
        }

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

        if (has_icons)
        {
            std::string cp_path;
            if (has_codepoints_path)
            {
                std::string path_str = entry["codepoints_path"].as<std::string>();
                fs::path p(path_str);
                if (!p.is_absolute())
                    p = manifest_dir / p;
                cp_path = p.lexically_normal().string();
                if (!fs::is_regular_file(cp_path))
                {
                    fprintf(
                        stderr,
                        "Font '%s': codepoints file not found: %s\n",
                        id.c_str(),
                        cp_path.c_str());
                    return 1;
                }
            }
            else
            {
                std::string url = entry["codepoints_url"].as<std::string>();
                if (cache_dir)
                {
                    fs::path cache_path
                        = fs::path(cache_dir) / (id + ".codepoints");
                    if (!fs::is_regular_file(cache_path.string()))
                    {
                        fs::create_directories(cache_dir);
                        if (download_url(url.c_str(), cache_path.string()) != 0)
                        {
                            fprintf(
                                stderr,
                                "Failed to download codepoints for '%s'\n",
                                id.c_str());
                            return 1;
                        }
                    }
                    cp_path = cache_path.lexically_normal().string();
                }
                else
                {
                    fs::path tmp = fs::temp_directory_path()
                        / ("alia_font_" + id + ".codepoints");
                    if (download_url(url.c_str(), tmp.string()) != 0)
                    {
                        fprintf(
                            stderr,
                            "Failed to download codepoints for '%s'\n",
                            id.c_str());
                        return 1;
                    }
                    cp_path = tmp.lexically_normal().string();
                }
            }

            std::unordered_map<std::string, uint32_t> cp_map;
            if (load_codepoints_map(cp_path, cp_map) != 0)
                return 1;

            std::vector<std::string> missing;
            std::unordered_set<std::string> seen_icon_names;
            std::unordered_set<std::string> seen_icon_suffixes;
            for (size_t k = 0; k < icons_node.size(); ++k)
            {
                std::string iname = icons_node[k].as<std::string>();
                if (!seen_icon_names.insert(iname).second)
                {
                    fprintf(
                        stderr,
                        "Font '%s': duplicate icon name '%s'\n",
                        id.c_str(),
                        iname.c_str());
                    return 1;
                }
                auto it = cp_map.find(iname);
                if (it == cp_map.end())
                    missing.push_back(iname);
                else
                {
                    std::string suf = icon_name_c_suffix(iname);
                    if (!seen_icon_suffixes.insert(suf).second)
                    {
                        fprintf(
                            stderr,
                            "Font '%s': icon names collapse to the same C++ "
                            "identifier suffix after sanitization (e.g. '%s')\n",
                            id.c_str(),
                            suf.c_str());
                        return 1;
                    }
                    fe.icon_codepoints.push_back({iname, it->second});
                }
            }
            if (!missing.empty())
            {
                fprintf(
                    stderr,
                    "Font '%s': icon name(s) not in codepoints file:",
                    id.c_str());
                for (std::string const& m : missing)
                    fprintf(stderr, " %s", m.c_str());
                fprintf(stderr, "\n");
                return 1;
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
        if (apply_variation_axes(font_entries[f], ft, font) != 0)
        {
            msdfgen::destroyFont(font);
            msdfgen::deinitializeFreetype(ft);
            return 1;
        }
        int loaded = 0;
        if (font_entries[f].icon_codepoints.empty())
        {
            loaded = font_geometry.loadCharset(
                font, font_scale, Charset::ASCII, true, true);
        }
        else
        {
            Charset charset;
            for (auto const& ic : font_entries[f].icon_codepoints)
                charset.add(static_cast<unicode_t>(ic.second));
            loaded = font_geometry.loadCharset(
                font, font_scale, charset, true, true);
        }
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
    packer.setPixelRange(msdfgen::Range(4.0));
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

#include <alia/abi/ui/msdf.h>

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
            std::string const& symid = font_entries[font_idx].id;
            f << "inline std::uint32_t const alia_font_" << symid
              << "_index = " << static_cast<std::uint32_t>(font_idx) << "u;\n";
            for (auto const& ic : font_entries[font_idx].icon_codepoints)
            {
                f << "inline std::uint32_t const alia_font_" << symid << "_icon_"
                  << icon_name_c_suffix(ic.first) << " = " << ic.second << "u;\n";
            }
        }

        float const dist_range
            = static_cast<float>(px_range.upper - px_range.lower);
        float const em_sz = static_cast<float>(em_size);

        f << R"(
inline std::size_t const alia_font_count = )"
          << fonts.size() << R"(;

extern alia_msdf_font_description const alia_font_descriptions[];

inline alia_msdf_font_description const& alia_font_description(std::size_t index) {
    return alia_font_descriptions[index];
}
)";
    }

    // --- Emit alia_fonts.cpp (glyph/kerning arrays and RLE data) ---
    {
        std::ofstream f(out_cpp);
        if (!f)
        {
            fprintf(stderr, "Cannot write %s\n", out_cpp);
            return 1;
        }
        f << "#include \"alia_fonts.h\"\n\n";

        for (size_t font_idx = 0; font_idx < fonts.size(); ++font_idx)
        {
            std::string const& symid = font_entries[font_idx].id;
            FontGeometry const& fg = fonts[font_idx];
            auto range = fg.getGlyphs();

            f << "alia_msdf_glyph const alia_font_" << symid
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
            f << "std::size_t const alia_font_" << symid
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
                f << "alia_msdf_kerning_pair const alia_font_" << symid
                  << "_kerning_pairs[] = {\n";
                for (size_t i = 0; i < kp_lines.size(); ++i)
                {
                    f << kp_lines[i];
                    f << (i + 1 < kp_lines.size() ? ",\n" : "\n");
                }
                f << "};\n";
            }
            f << "std::size_t const alia_font_" << symid
              << "_kerning_pair_count = " << kp_lines.size() << ";\n\n";

        }

        float const dist_range
            = static_cast<float>(px_range.upper - px_range.lower);
        float const em_sz = static_cast<float>(em_size);
        f << "alia_msdf_font_description const alia_font_descriptions[] = {\n";
        for (size_t font_idx = 0; font_idx < fonts.size(); ++font_idx)
        {
            std::string const& symid = font_entries[font_idx].id;
            FontGeometry const& fg = fonts[font_idx];
            msdfgen::FontMetrics const& m = fg.getMetrics();
            f << "    { {" << std::showpoint << static_cast<float>(m.emSize)
              << "f, " << static_cast<float>(m.lineHeight) << "f, "
              << static_cast<float>(m.ascenderY) << "f, "
              << static_cast<float>(m.descenderY) << "f, "
              << static_cast<float>(m.underlineY) << "f, "
              << static_cast<float>(m.underlineThickness) << "f}, {"
              << dist_range << "f, 0.f, " << em_sz << "f, "
              << static_cast<float>(w) << "f, " << static_cast<float>(h)
              << "f}, alia_font_" << symid << "_glyphs, alia_font_" << symid
              << "_glyph_count, "
              << (fonts[font_idx].getKerning().empty()
                      ? "nullptr"
                      : std::string("alia_font_") + symid + "_kerning_pairs")
              << ", alia_font_" << symid << "_kerning_pair_count },\n";
        }
        f << "};\n\n";

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
          << ";\n\n";
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
