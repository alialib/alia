#include <alia/core/common.hpp>

namespace alia {

blob
make_blob(std::string s)
{
    auto storage = std::make_shared<std::string>(std::move(s));
    std::byte* const data = reinterpret_cast<std::byte*>(storage->data());
    auto const size = storage->size();
    return blob{std::shared_ptr<std::byte[]>(std::move(storage), data), size};
}

} // namespace alia
