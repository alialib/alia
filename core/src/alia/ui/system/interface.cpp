#include <alia/system/interface.hpp>

#include <chrono>

#include <alia/abi/base/arena.h>
#include <alia/abi/events.h>
#include <alia/flow/dispatch.hpp>
#include <alia/system/object.hpp>

namespace alia {

bool
system_needs_refresh(ui_system& sys)
{
    // TODO
    // return sys.refresh_needed;
    return true;
}

void
refresh_system(ui_system& sys)
{
    // TODO

    // sys.refresh_needed = false;
    // ++sys.refresh_counter;

    // std::chrono::steady_clock::time_point begin
    //     = std::chrono::steady_clock::now();

    {
        auto refresh_event = alia_make_refresh_event({});
        dispatch_event(sys, refresh_event);
    }

    // long long refresh_time;
    // {
    //     std::chrono::steady_clock::time_point end
    //         = std::chrono::steady_clock::now();
    //     refresh_time =
    //     std::chrono::duration_cast<std::chrono::microseconds>(
    //                        end - begin)
    //                        .count();
    // }

    // static long long max_refresh_time = 0;
    // max_refresh_time = (std::max)(refresh_time, max_refresh_time);
    // std::cout << "refresh: " << refresh_time << "[us]\n";
    // std::cout << "max_refresh_time: " << max_refresh_time << "[us]\n";
}

void
set_error_handler(
    ui_system& sys, std::function<void(std::exception_ptr)> handler)
{
    // TODO
    // sys.error_handler = handler;
}

void
schedule_asynchronous_update(ui_system& sys, std::function<void()> update)
{
    // TODO
    // sys.external->schedule_asynchronous_update(std::move(update));
}

} // namespace alia
