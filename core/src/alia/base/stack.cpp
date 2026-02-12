#include <alia/abi/base/stack.h>

#include <alia/base/stack.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <new>

// TODO: Split out into utility files.

static inline bool
alia_is_pow2_u32(uint32_t x)
{
    return x && ((x & (x - 1u)) == 0u);
}

static inline uintptr_t
alia_align_up_uintptr(uintptr_t v, uint32_t align)
{
    ALIA_ASSERT(alia_is_pow2_u32(align));
    uintptr_t mask = uintptr_t(align - 1u);
    return (v + mask) & ~mask;
}

static inline uint32_t
alia_align_up_u32(uint32_t v, uint32_t align)
{
    ALIA_ASSERT(alia_is_pow2_u32(align));
    uint32_t mask = align - 1u;
    return (v + mask) & ~mask;
}

static inline bool
alia_is_aligned_ptr(void const* p, uint32_t align)
{
    ALIA_ASSERT(alia_is_pow2_u32(align));
    return (reinterpret_cast<uintptr_t>(p) & uintptr_t(align - 1u)) == 0u;
}

// Reserve exactly ALIA_MIN_ALIGN bytes at the start of each entry for the
// header area.
static constexpr uint32_t ALIA_STACK_HEADER_AREA
    = uint32_t(sizeof(alia_stack_entry_header));

// Largest entry size that fits in 16 bits and is a multiple of ALIA_MIN_ALIGN.
static constexpr uint32_t ALIA_STACK_MAX_ENTRY_SIZE_U32
    = (0xFFFFu & ~(uint32_t(ALIA_MIN_ALIGN) - 1u));

static_assert(
    (ALIA_STACK_HEADER_AREA % ALIA_MIN_ALIGN) == 0,
    "header area must be min-aligned");

static inline alia_stack_entry_header*
alia_stack_header_at(uint8_t* entry_start)
{
    return reinterpret_cast<alia_stack_entry_header*>(entry_start);
}

static inline alia_stack_entry_header const*
alia_stack_header_at(uint8_t const* entry_start)
{
    return reinterpret_cast<alia_stack_entry_header const*>(entry_start);
}

static inline uint8_t*
alia_stack_top_entry_start(alia_stack const* s)
{
    ALIA_ASSERT(s->top_entry_size != 0);
    ALIA_ASSERT(s->top >= s->top_entry_size);
    return s->base + (s->top - s->top_entry_size);
}

// API IMPLEMENTATION

extern "C" {

alia_struct_spec
alia_stack_object_spec(void)
{
    alia_struct_spec spec;
    spec.size = sizeof(alia_stack);
    spec.align = alignof(alia_stack);
    return spec;
}

alia_stack*
alia_stack_init(void* object_storage, void* buffer, size_t capacity)
{
    ALIA_ASSERT(object_storage);
    ALIA_ASSERT(buffer);

    // Capacity must be representable by our 32-bit cursor.
    ALIA_ASSERT(capacity <= size_t(0xFFFF'FFFFu));

    // Backing buffer must be aligned enough to satisfy all supported
    // alignments.
    ALIA_ASSERT(alia_is_aligned_ptr(buffer, ALIA_MAX_ALIGN));
    ALIA_ASSERT((ALIA_MAX_ALIGN % ALIA_MIN_ALIGN) == 0);

    alia_stack* s = new (object_storage) alia_stack;
    s->base = static_cast<uint8_t*>(buffer);
    s->capacity = capacity;
    s->top = 0;
    s->top_entry_size = 0;
    return s;
}

void
alia_stack_cleanup(alia_stack* s)
{
    if (!s)
        return;

    s->base = nullptr;
    s->capacity = 0;
    s->top = 0;
    s->top_entry_size = 0;

    s->~alia_stack();
}

void
alia_stack_reset(alia_stack* s)
{
    ALIA_ASSERT(s);
    s->top = 0;
    s->top_entry_size = 0;
}

void*
alia_stack_push(
    alia_stack* s, uint16_t payload_size, alia_stack_vtable const* vt)
{
    // In this path, `payload_size` must be a multiple of `ALIA_MIN_ALIGN`.
    ALIA_ASSERT((payload_size % ALIA_MIN_ALIGN) == 0);

    ALIA_ASSERT(s);
    ALIA_ASSERT(s->base);

    // Validate payload size constraint.
    ALIA_ASSERT(
        payload_size
        <= 0xFFFFu - ALIA_STACK_HEADER_AREA - ALIA_MIN_ALIGN - ALIA_MAX_ALIGN);

    ALIA_ASSERT((s->top % ALIA_MIN_ALIGN) == 0);
    uint32_t entry_start_off = s->top;
    uint8_t* entry_start = s->base + entry_start_off;

    uint32_t payload_off = entry_start_off + ALIA_STACK_HEADER_AREA;

    uint32_t entry_size_u32 = ALIA_STACK_HEADER_AREA + payload_size;
    uint32_t entry_end_off = payload_off + payload_size;

    // Capacity check.
    // TODO: Handle overflow.
    ALIA_ASSERT(size_t(entry_end_off) <= s->capacity);

    // Write header.
    alia_stack_entry_header* h = alia_stack_header_at(entry_start);
    h->prev_entry_size = s->top_entry_size; // 0 if empty
    h->payload_offset = uint16_t(payload_off - entry_start_off);
    h->payload_size = uint16_t(payload_size);
    h->reserved = 0;
    h->vtable = vt;

    // Advance stack state.
    s->top = entry_end_off;
    s->top_entry_size = uint16_t(entry_size_u32);

    return s->base + payload_off;
}

void*
alia_stack_push_aligned(
    alia_stack* s,
    uint16_t payload_size,
    uint16_t payload_align,
    alia_stack_vtable const* vt)
{
    ALIA_ASSERT(s);
    ALIA_ASSERT(s->base);

    // Validate alignment constraints.
    ALIA_ASSERT(payload_align != 0);
    ALIA_ASSERT(alia_is_pow2_u32(payload_align));
    ALIA_ASSERT(payload_align <= ALIA_MAX_ALIGN);

    // Validate payload size constraint.
    ALIA_ASSERT(
        payload_size
        <= 0xFFFFu - ALIA_STACK_HEADER_AREA - ALIA_MIN_ALIGN - ALIA_MAX_ALIGN);

    // Entry starts at current top (maintained ALIA_MIN_ALIGN-aligned).
    ALIA_ASSERT((s->top % ALIA_MIN_ALIGN) == 0);
    uint32_t entry_start_off = s->top;
    uint8_t* entry_start = s->base + entry_start_off;

    // Compute payload start, beginning after the reserved header area.
    uint32_t header_end_off = entry_start_off + ALIA_STACK_HEADER_AREA;

    uintptr_t payload_addr = alia_align_up_uintptr(
        reinterpret_cast<uintptr_t>(s->base) + uintptr_t(header_end_off),
        payload_align);

    uint32_t payload_off
        = uint32_t(payload_addr - reinterpret_cast<uintptr_t>(s->base));
    ALIA_ASSERT(payload_off >= header_end_off);

    // Compute total entry size (rounded up to ALIA_MIN_ALIGN).
    uint32_t end_off_unaligned = payload_off + payload_size;
    uint32_t entry_end_off
        = alia_align_up_u32(end_off_unaligned, uint32_t(ALIA_MIN_ALIGN));
    uint32_t entry_size_u32 = entry_end_off - entry_start_off;

    // Enforce 16-bit entry size and alignment.
    ALIA_ASSERT((entry_size_u32 % ALIA_MIN_ALIGN) == 0);
    ALIA_ASSERT(entry_size_u32 <= ALIA_STACK_MAX_ENTRY_SIZE_U32);
    ALIA_ASSERT(entry_size_u32 <= 0xFFFFu);

    // Capacity check.
    // TODO: Handle overflow.
    ALIA_ASSERT(size_t(entry_end_off) <= s->capacity);

    // Write header.
    alia_stack_entry_header* h = alia_stack_header_at(entry_start);
    h->prev_entry_size = s->top_entry_size; // 0 if empty
    h->payload_offset = uint16_t(payload_off - entry_start_off);
    h->payload_size = uint16_t(payload_size);
    h->reserved = 0;
    h->vtable = vt;

    // Advance stack state.
    s->top = entry_end_off;
    s->top_entry_size = uint16_t(entry_size_u32);

    return s->base + payload_off;
}

alia_stack_entry_header const*
alia_stack_peek_header(alia_stack const* s)
{
    ALIA_ASSERT(s);
    if (s->top_entry_size == 0)
        return nullptr;

    uint8_t const* entry_start = alia_stack_top_entry_start(s);
    return alia_stack_header_at(entry_start);
}

void*
alia_stack_peek_payload(alia_stack const* s)
{
    alia_stack_entry_header const* h = alia_stack_peek_header(s);
    if (!h)
        return nullptr;

    uint8_t* entry_start = alia_stack_top_entry_start(s);
    return entry_start + h->payload_offset;
}

void
alia_stack_pop(alia_stack* s)
{
    ALIA_ASSERT(s);

    if (s->top_entry_size == 0)
        return; // Or assert, depending on your preference.

    // Locate current top entry start and read prev size before mutating.
    uint8_t* entry_start = alia_stack_top_entry_start(s);
    alia_stack_entry_header const* h = alia_stack_header_at(entry_start);

    uint16_t cur_size = s->top_entry_size;
    uint16_t prev_size = h->prev_entry_size;

    // Sanity checks.
    ALIA_ASSERT(cur_size != 0);
    ALIA_ASSERT((cur_size % ALIA_MIN_ALIGN) == 0);

    // Pop.
    s->top -= cur_size;
    s->top_entry_size = prev_size;
}

} // extern "C"
