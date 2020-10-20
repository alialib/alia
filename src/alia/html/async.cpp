#include <alia/html/async.hpp>

#include <emscripten/emscripten.h>

namespace alia {
namespace web {

struct async_call_data
{
    std::function<void()> func;
};

static void
async_call_callback(void* data_arg)
{
    std::unique_ptr<async_call_data> data(
        reinterpret_cast<async_call_data*>(data_arg));
    data->func();
}

void
async_call(std::function<void()> func, int millis)
{
    auto* data = new async_call_data{std::move(func)};
    emscripten_async_call(async_call_callback, data, millis);
}

} // namespace web
} // namespace alia
