#include "assets.hpp"

#include <filesystem>
#include <fstream>

namespace alia {

blob
load_asset(std::string_view path)
{
    // In dev mode, just load the assets directly from the source directory.
    // Get the directory containing this source file
    auto const source_dir = std::filesystem::path(__FILE__).parent_path();
    auto const asset_path = source_dir / "assets" / path;

    // Open the file and get its size.
    std::ifstream file(asset_path, std::ios::binary | std::ios::ate);
    if (!file)
        throw exception("failed to open asset file: " + std::string(path));
    auto size = file.tellg();

    // Allocate a buffer and read the file into it.
    auto data = std::make_shared<std::byte[]>(size);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(data.get()), size);

    return blob{std::move(data), static_cast<uint64_t>(size)};
}

} // namespace alia
