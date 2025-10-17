#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// PANIC REASONS

typedef enum alia_panic_reason
{
    ALIA_PANIC_OUT_OF_MEMORY = 1, // out of memory
    ALIA_PANIC_API_MISUSE, // API misuse detected
    ALIA_PANIC_INTERNAL // internal invariant failed
} alia_panic_reason;

// PANIC INFO

typedef struct alia_panic_info
{
    alia_panic_reason reason;
    char const* subsystem; // e.g., "scratch"
    char const* msg; // short human text (static or ephemeral OK)
} alia_panic_info;

// PANIC HANDLER TYPE (MUST NOT RETURN)

typedef void (*alia_panic_handler_fn)(void* user, alia_panic_info const* info);

// Set/replace the global panic handler. Passing NULL restores the default.
// TODO: Add system/context-specific panic handlers.
void
alia_set_panic_handler(alia_panic_handler_fn fn, void* user);

// Trigger a panic (does not return). Calls the handler; if it returns, aborts.
#if defined(__GNUC__) || defined(__clang__)
__attribute__((noreturn))
#endif
void
alia_panic_now(alia_panic_info const* info);

#ifdef __cplusplus
}
#endif
