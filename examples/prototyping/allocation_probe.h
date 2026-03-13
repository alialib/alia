// alloc_probe.h
#ifdef _MSC_VER
#include <atomic>
#include <crtdbg.h>
#include <functional>
#include <windows.h>
#endif

struct AllocProbeResult
{
    bool any; // did any allocations occur?
    long first_req = -1; // first global request number seen
    long last_req = -1; // last global request number seen
    unsigned count = 0; // number of alloc/realloc events seen
};

#if 0

#ifdef _MSC_VER
namespace detail {
// Per-thread tracking (the hook is process-wide, so we filter by thread)
static thread_local bool g_active = false;
static thread_local DWORD g_tid = 0;
static thread_local long g_first = -1;
static thread_local long g_last = -1;
static thread_local unsigned g_count = 0;

// Keep previous hook so we can restore it
static _CRT_ALLOC_HOOK g_prevHook = nullptr;

// Our hook: called by the Debug CRT on each alloc/free/realloc
static int __cdecl Hook(
    int allocType,
    void* /*userData*/,
    size_t /*size*/,
    int /*blockType*/,
    long requestNumber,
    const unsigned char* /*filename*/,
    int /*line*/)
{
    if (g_active && GetCurrentThreadId() == g_tid)
    {
        if (allocType == _HOOK_ALLOC || allocType == _HOOK_REALLOC)
        {
            if (g_first < 0)
                g_first = requestNumber; // <- global order id
            g_last = requestNumber;
            ++g_count;
        }
    }
    return g_prevHook
             ? g_prevHook(allocType, nullptr, 0, 0, requestNumber, nullptr, 0)
             : 1;
}

struct Activator
{
    Activator()
    {
        // ensure debug heap is on so hooks fire
        int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        _CrtSetDbgFlag(flags | _CRTDBG_ALLOC_MEM_DF);
        g_prevHook = _CrtSetAllocHook(&Hook);
    }
    ~Activator()
    {
        _CrtSetAllocHook(g_prevHook);
    }
};
} // namespace detail

#endif // _MSC_VER

template<class F>
AllocProbeResult
probe_allocations(F&& f)
{
#ifdef _MSC_VER
    detail::g_active = true;
    detail::g_tid = GetCurrentThreadId();
    detail::g_first = -1;
    detail::g_last = -1;
    detail::g_count = 0;
    detail::Activator guard;

    // Run the code under test
    std::forward<F>(f)();

    AllocProbeResult r;
    r.any = detail::g_count > 0;
    r.first_req = detail::g_first;
    r.last_req = detail::g_last;
    r.count = detail::g_count;

    detail::g_active = false;
    return r;
#else
    // Non-MSVC / Release: can’t use Debug CRT hook; report no info.
    return AllocProbeResult{false, -1, -1, 0};
#endif
}

#else

template<class F>
AllocProbeResult
probe_allocations(F&& f)
{
    std::forward<F>(f)();
    return AllocProbeResult{false, -1, -1, 0};
}

#endif
