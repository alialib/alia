#ifndef ALIA_ABI_BASE_STACK_H
#define ALIA_ABI_BASE_STACK_H

#include <alia/abi/base/arena.h>
#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

typedef struct alia_stack_vtable alia_stack_vtable;

typedef struct alia_stack
{
    uint8_t* base;
    size_t capacity;
    // write cursor in bytes from base (always aligned to ALIA_MIN_ALIGN)
    uint32_t top;
    // size of the top entry in bytes (0 if the stack is empty)
    uint16_t top_entry_size;
    // high-water mark of `top` since this cursor was opened
    size_t peak;
    alia_grow_callback grow;
} alia_stack;

// Bind `s` to a caller-owned buffer. `buffer` must be aligned to
// ALIA_MAX_ALIGN, and `capacity` must fit in 32 bits. The caller frees
// `buffer`.
static inline void
alia_stack_bind(
    alia_stack* s, void* buffer, size_t capacity, alia_grow_callback grow)
{
    ALIA_ASSERT(s);
    ALIA_ASSERT(buffer);
    ALIA_ASSERT(capacity <= (size_t) 0xFFFFFFFFu);
    ALIA_ASSERT(
        ((uintptr_t) buffer & (uintptr_t) (ALIA_MAX_ALIGN - 1u)) == 0u);

    s->base = (uint8_t*) buffer;
    s->capacity = capacity;
    s->top = 0;
    s->top_entry_size = 0;
    s->peak = 0;
    s->grow = grow;
}

// Reset the stack to empty. The local peak is left alone.
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
