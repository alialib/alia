#include <alia/abi/ui/trace.h>
#include <alia/ui/system/object.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <chrono>
#endif

namespace {

#if defined(_WIN32)
LARGE_INTEGER
qpc_frequency()
{
    static LARGE_INTEGER freq = {};
    static bool initialized = false;
    if (!initialized)
    {
        QueryPerformanceFrequency(&freq);
        initialized = true;
    }
    return freq;
}
#endif

} // namespace

extern "C" {

alia_trace_tick
alia_trace_now(void)
{
#if defined(_WIN32)
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<alia_trace_tick>(counter.QuadPart);
#else
    auto const now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<alia_trace_tick>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
#endif
}

int64_t
alia_trace_ticks_to_ns(alia_trace_tick ticks)
{
    if (ticks <= 0)
        return 0;
#if defined(_WIN32)
    LARGE_INTEGER const freq = qpc_frequency();
    // (ticks * 1e9) / freq, with intermediate overflow avoidance
    return static_cast<int64_t>(
        (static_cast<long double>(ticks) * 1'000'000'000.0L)
        / static_cast<long double>(freq.QuadPart));
#else
    return static_cast<int64_t>(ticks);
#endif
}

void
alia_ui_set_trace_sink(alia_ui_system* ui, alia_ui_trace_sink const* sink)
{
    ALIA_ASSERT(ui);
    ui->trace_sink = sink;
}

alia_ui_trace_sink const*
alia_ui_get_trace_sink(alia_ui_system const* ui)
{
    ALIA_ASSERT(ui);
    return ui->trace_sink;
}

void
alia_ui_trace_frame_begin(alia_ui_system* ui)
{
    ALIA_ASSERT(ui);
    if (ui->trace_sink && ui->trace_sink->frame_begin)
        ui->trace_sink->frame_begin(ui->trace_sink->user);
}

void
alia_ui_trace_frame_end(alia_ui_system* ui)
{
    ALIA_ASSERT(ui);
    if (ui->trace_sink && ui->trace_sink->frame_end)
        ui->trace_sink->frame_end(ui->trace_sink->user);
}

void
alia_ui_trace_emit_pass(alia_ui_system* ui, alia_trace_pass const* pass)
{
    ALIA_ASSERT(ui);
    ALIA_ASSERT(pass);
    if (ui->trace_sink && ui->trace_sink->pass)
        ui->trace_sink->pass(ui->trace_sink->user, pass);
}

} // extern "C"
