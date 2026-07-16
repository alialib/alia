// Offline Slang baker: .slang → generated C header/source with shader blob.
//
// Usage:
//   alia_shader_builder <input.slang> <out.h> <out.cpp>
//       --format dxbc|glsl-es
//       --entry <name>
//       --symbol <prefix>
//       --slangc <path-to-slangc>
//       [--include-dir <dir>]...

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct options
{
    fs::path input;
    fs::path out_h;
    fs::path out_cpp;
    std::string format; // "dxbc" or "glsl-es"
    std::string entry = "ps_main";
    std::string symbol = "alia_effect";
    fs::path slangc;
    std::vector<fs::path> include_dirs;
};

void
usage()
{
    std::cerr
        << "usage: alia_shader_builder <input.slang> <out.h> <out.cpp>\n"
           "  --format dxbc|glsl-es\n"
           "  --entry <name>           (default: ps_main)\n"
           "  --symbol <prefix>\n"
           "  --slangc <path>\n"
           "  --include-dir <dir>      (repeatable)\n";
}

bool
parse_args(int argc, char** argv, options& opt)
{
    if (argc < 4)
        return false;
    opt.input = argv[1];
    opt.out_h = argv[2];
    opt.out_cpp = argv[3];
    for (int i = 4; i < argc; ++i)
    {
        std::string_view a = argv[i];
        auto need = [&](char const* flag) -> char const* {
            if (i + 1 >= argc)
            {
                std::cerr << "missing value for " << flag << "\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--format")
            opt.format = need("--format");
        else if (a == "--entry")
            opt.entry = need("--entry");
        else if (a == "--symbol")
            opt.symbol = need("--symbol");
        else if (a == "--slangc")
            opt.slangc = need("--slangc");
        else if (a == "--include-dir")
            opt.include_dirs.push_back(need("--include-dir"));
        else
        {
            std::cerr << "unknown argument: " << a << "\n";
            return false;
        }
    }
    if (opt.format != "dxbc" && opt.format != "glsl-es")
    {
        std::cerr << "--format must be dxbc or glsl-es\n";
        return false;
    }
    if (opt.slangc.empty())
    {
        std::cerr << "--slangc is required\n";
        return false;
    }
    return true;
}

std::string
quote(fs::path const& p)
{
    // Prefer generic (forward-slash) paths; quote only when needed.
    std::string s = p.generic_string();
    if (s.find(' ') != std::string::npos)
        return "\"" + s + "\"";
    return s;
}

int
run_command(std::vector<std::string> const& args)
{
    std::ostringstream cmd;
#ifdef _WIN32
    // cmd.exe /c requires an extra pair of quotes when the exe is quoted.
    cmd << "cmd /c \"";
#endif
    for (size_t i = 0; i < args.size(); ++i)
    {
        if (i)
            cmd << ' ';
        cmd << args[i];
    }
#ifdef _WIN32
    cmd << "\"";
#endif
    std::cerr << "[alia_shader_builder] " << cmd.str() << "\n";
    return std::system(cmd.str().c_str());
}

std::string
read_file_binary_as_string(fs::path const& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        std::cerr << "failed to read " << path << "\n";
        std::exit(1);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::vector<std::uint8_t>
read_file_bytes(fs::path const& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
    {
        std::cerr << "failed to read " << path << "\n";
        std::exit(1);
    }
    return std::vector<std::uint8_t>(
        std::istreambuf_iterator<char>(in),
        std::istreambuf_iterator<char>());
}

// Downlevel Slang's GLSL 450 to a body-only GLSL ES fragment for Alia's GL
// loader (which prepends #version 300 es / 330 core).
std::string
glsl_to_es_body(std::string src)
{
    std::istringstream in(src);
    std::ostringstream out;
    std::string line;
    while (std::getline(in, line))
    {
        // Drop version and Slang row-major defaults (invalid / redundant on ES).
        if (line.rfind("#version", 0) == 0)
            continue;
        if (line.find("layout(row_major) uniform;") != std::string::npos)
            continue;
        if (line.find("layout(row_major) buffer;") != std::string::npos)
            continue;
        // WebGL2 / GLES3.0: set UBO bindings via glUniformBlockBinding.
        if (line.find("layout(binding") != std::string::npos
            && line.find("uniform") == std::string::npos)
            continue;
        // Combined: layout(binding = N) layout(std140) uniform ...
        auto bind = line.find("layout(binding");
        if (bind != std::string::npos)
        {
            auto end = line.find(')', bind);
            if (end != std::string::npos)
            {
                line.erase(bind, end - bind + 1);
                // Collapse leftover spaces.
                while (!line.empty() && line[0] == ' ')
                    line.erase(line.begin());
            }
        }
        out << line << '\n';
    }
    return out.str();
}

void
write_generated(
    options const& opt,
    std::vector<std::uint8_t> const& bytes,
    char const* format_enum,
    bool nul_terminate)
{
    fs::create_directories(opt.out_h.parent_path());
    fs::create_directories(opt.out_cpp.parent_path());

    std::string const bytes_sym = opt.symbol + "_shader_bytes";
    std::string const size_sym = opt.symbol + "_shader_size";
    std::string const format_sym = opt.symbol + "_shader_format";
    std::string const desc_fn = opt.symbol + "_desc";

    {
        std::ofstream h(opt.out_h);
        h << "// Generated by alia_shader_builder — do not edit.\n"
          << "#ifndef " << opt.symbol << "_GENERATED_H\n"
          << "#define " << opt.symbol << "_GENERATED_H\n\n"
          << "#include <alia/abi/ui/effects.h>\n"
          << "#include <stddef.h>\n\n"
          << "extern unsigned char const " << bytes_sym << "[];\n"
          << "extern size_t const " << size_sym << ";\n"
          << "extern alia_shader_format const " << format_sym << ";\n\n"
          << "// `params_size` is the CPU/GPU params POD size (typically\n"
          << "// sizeof(your_params_struct)).\n"
          << "static inline alia_effect_desc\n"
          << desc_fn << "(size_t params_size)\n"
          << "{\n"
          << "    alia_effect_desc desc;\n"
          << "    desc.shader.format = " << format_sym << ";\n"
          << "    desc.shader.data = " << bytes_sym << ";\n"
          << "    desc.shader.size = " << size_sym << ";\n"
          << "    desc.params_size = params_size;\n"
          << "    return desc;\n"
          << "}\n\n"
          << "#endif\n";
    }

    size_t const payload_size = bytes.size();

    {
        std::ofstream cpp(opt.out_cpp);
        cpp << "// Generated by alia_shader_builder — do not edit.\n"
            << "#include \"" << opt.out_h.filename().string() << "\"\n\n"
            << "unsigned char const " << bytes_sym << "[] = {\n";
        for (size_t i = 0; i < bytes.size(); ++i)
        {
            if (i % 12 == 0)
                cpp << "    ";
            cpp << "0x" << std::hex << std::uppercase
                << int(bytes[i]) << std::dec << ",";
            if (i % 12 == 11 || i + 1 == bytes.size())
                cpp << "\n";
            else
                cpp << " ";
        }
        if (nul_terminate)
            cpp << "    0x00\n";
        cpp << "};\n\n"
            << "size_t const " << size_sym << " = " << payload_size << ";\n"
            << "alia_shader_format const " << format_sym << " = " << format_enum
            << ";\n";
    }
}

} // namespace

int
main(int argc, char** argv)
{
    options opt;
    if (!parse_args(argc, argv, opt))
    {
        usage();
        return 2;
    }

    fs::path const tmp_dir = opt.out_cpp.parent_path() / (opt.symbol + "_slang_tmp");
    fs::create_directories(tmp_dir);
    fs::path const raw_out = tmp_dir / (opt.format == "dxbc" ? "out.dxbc" : "out.glsl");

    std::vector<std::string> args;
    args.push_back(quote(opt.slangc));
    args.push_back(quote(opt.input));
    args.push_back("-entry");
    args.push_back(opt.entry);
    args.push_back("-stage");
    args.push_back("fragment");
    for (auto const& d : opt.include_dirs)
    {
        args.push_back("-I");
        args.push_back(quote(d));
    }

    if (opt.format == "dxbc")
    {
        args.push_back("-target");
        args.push_back("dxbc");
        args.push_back("-profile");
        args.push_back("sm_5_0");
        args.push_back("-o");
        args.push_back(quote(raw_out));
    }
    else
    {
        // Slang emits desktop GLSL; we downlevel to an ES-friendly body.
        args.push_back("-target");
        args.push_back("glsl");
        args.push_back("-profile");
        args.push_back("glsl_450");
        args.push_back("-fvk-b-shift");
        args.push_back("0");
        args.push_back("0");
        args.push_back("-o");
        args.push_back(quote(raw_out));
    }

    int const rc = run_command(args);
    if (rc != 0)
    {
        std::cerr << "slangc failed with exit code " << rc << "\n";
        return 1;
    }

    if (opt.format == "dxbc")
    {
        auto bytes = read_file_bytes(raw_out);
        write_generated(opt, bytes, "ALIA_SHADER_FORMAT_DXBC", false);
    }
    else
    {
        std::string body = glsl_to_es_body(read_file_binary_as_string(raw_out));
        std::vector<std::uint8_t> bytes(body.begin(), body.end());
        write_generated(opt, bytes, "ALIA_SHADER_FORMAT_GLSL_ES", true);
    }

    std::cerr << "[alia_shader_builder] wrote " << opt.out_h << " and "
              << opt.out_cpp << "\n";
    return 0;
}
