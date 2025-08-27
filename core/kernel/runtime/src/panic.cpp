#include <alia/panic.h>
#include <cstdio>
#include <cstdlib>

static alia_panic_handler_fn g_handler = nullptr;
static void* g_handler_user = nullptr;

extern "C" {

void
alia_set_panic_handler(alia_panic_handler_fn fn, void* user)
{
    g_handler = fn;
    g_handler_user = user;
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((noreturn))
#endif
void
alia_panic_now(alia_panic_info const* info)
{
    if (g_handler)
    {
        g_handler(g_handler_user, info);
        // Handler shouldn't return.
    }

    // Either there was no handler or the handler returned.
    // Print a terse line and abort.
    const char* sub = info->subsystem ? info->subsystem : "alia";
    const char* msg = info->msg ? info->msg : "(no message)";
    std::fprintf(
        stderr, "PANIC[%s]: %s (reason=%d)\n", sub, msg, info->reason);
    std::abort();
}

} // extern "C"
