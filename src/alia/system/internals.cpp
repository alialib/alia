#include <alia/system/internals.hpp>

#include <chrono>

namespace alia {

millisecond_count
get_default_tick_count()
{
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<
               std::chrono::duration<millisecond_count, std::milli>>(
               now - start)
        .count();
}

} // namespace alia
