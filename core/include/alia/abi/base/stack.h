#ifndef ALIA_ABI_BASE_STACK_H
#define ALIA_ABI_BASE_STACK_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_stack alia_stack;
typedef struct alia_stack_vtable alia_stack_vtable;

// LIFECYCLE

alia_struct_spec
alia_stack_object_spec(void);

// Initialize the stack over a caller-provided buffer.
// The buffer must be large enough to hold the stack and all its entries
// and it must be aligned to ALIA_MAX_STACK_ALIGN.
ALIA_API alia_stack*
alia_stack_init(void* object_storage, void* buffer, size_t capacity);

// Clean up the stack.
ALIA_API void
alia_stack_cleanup(alia_stack* s);

// Reset the stack to empty.
ALIA_API void
alia_stack_reset(alia_stack* s);

// PUSH/PEEK/POP

// `ALIA_STACK_MAX_ENTRY_SIZE` is the largest possible size for an entry in the
// stack (including overhead).
// An entry must:
// - fit in uint16_t
// - be a multiple of ALIA_MIN_ALIGN
#define ALIA_STACK_MAX_ENTRY_SIZE                                             \
    ((uint16_t) (0xFFFFu & ~(ALIA_MIN_ALIGN - 1u)))

// Push an entry with ALIA_MIN_ALIGN alignment and return a pointer to the
// payload bytes. The payload_size must be a multiple of ALIA_MIN_ALIGN.
ALIA_API void*
alia_stack_push(
    alia_stack* s, uint16_t payload_size, alia_stack_vtable const* vt);

// Push an entry with a specific alignment and return a pointer to the payload
// bytes.
ALIA_API void*
alia_stack_push_aligned(
    alia_stack* s,
    uint16_t payload_size,
    uint16_t payload_align,
    alia_stack_vtable const* vt);

typedef struct alia_stack_entry_header
{
    // size of the previous entry (0 if this is the bottom entry)
    uint16_t prev_entry_size;

    // offset (in bytes) from the start of this entry to the payload
    uint16_t payload_offset;

    // payload size (in bytes) - primarily for tools/introspection
    uint16_t payload_size;

    // reserved for future use (flags, debug cookie, etc.)
    uint16_t reserved;

    // nullable (for introspection/debugging/future use)
    alia_stack_vtable const* vtable;
} alia_stack_entry_header;

// Peek at the header of the top entry.
ALIA_API alia_stack_entry_header const*
alia_stack_peek_header(alia_stack const* s);

// Peek at the payload of the top entry.
ALIA_API void*
alia_stack_peek_payload(alia_stack const* s);

// Pop the top entry.
ALIA_API void
alia_stack_pop(alia_stack* s);

// INTROSPECTION

struct alia_stack_vtable
{
    char const* name; // e.g. "transform", "clip", "style"
    // Called by debug tools to print/inspect one entry. May be NULL.
    void (*describe)(void const* payload, uint16_t payload_size, void* out);
};

ALIA_EXTERN_C_END

#endif /* ALIA_ABI_BASE_STACK_H */
