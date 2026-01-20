#include <alia/abi/stack.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

// --------- helpers ---------
// TODO: Split out into utility files.

static int
is_pow2_u32(uint32_t x)
{
    return x && ((x & (x - 1u)) == 0u);
}

static int
is_aligned_ptr(void const* p, uint32_t align)
{
    TEST_ASSERT(is_pow2_u32(align));
    return (((uintptr_t) p) & (uintptr_t) (align - 1u)) == 0u;
}

static void*
aligned_alloc_portable(size_t align, size_t size)
{
#if defined(_MSC_VER)
    return _aligned_malloc(size, align);
#else
    // C11 aligned_alloc requires size to be a multiple of align.
    size_t rounded = (size + align - 1u) / align * align;
    return aligned_alloc(align, rounded);
#endif
}

static void
aligned_free_portable(void* p)
{
#if defined(_MSC_VER)
    _aligned_free(p);
#else
    free(p);
#endif
}

// minimal vtable for tests
static void
dummy_describe(void const* payload, uint16_t payload_size, void* out)
{
    (void) payload;
    (void) payload_size;
    (void) out;
}

static alia_stack_vtable const VT_A = {"A", dummy_describe};
static alia_stack_vtable const VT_B = {"B", dummy_describe};

// --------- tests ---------

static void
test_stack_lifecycle_reset_empty(void)
{
    alia_struct_spec spec = alia_stack_object_spec();
    TEST_ASSERT(spec.size > 0);
    TEST_ASSERT(spec.align > 0);

    void* obj = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(obj != NULL);
    memset(obj, 0xCD, spec.size);

    size_t cap = 64 * 1024;
    void* buf = aligned_alloc_portable(ALIA_MAX_ALIGN, cap);
    TEST_ASSERT(buf != NULL);
    memset(buf, 0xEF, cap);

    alia_stack* s = alia_stack_init(obj, buf, cap);
    TEST_ASSERT(s != NULL);

    // Empty peek should be NULL.
    TEST_ASSERT(alia_stack_peek_header(s) == NULL);
    TEST_ASSERT(alia_stack_peek_payload(s) == NULL);

    // Pop on empty should not crash and should remain empty.
    alia_stack_pop(s);
    TEST_ASSERT(alia_stack_peek_header(s) == NULL);

    // Reset should keep it empty.
    alia_stack_reset(s);
    TEST_ASSERT(alia_stack_peek_header(s) == NULL);

    alia_stack_cleanup(s);
    aligned_free_portable(buf);
    aligned_free_portable(obj);
}

static void
test_stack_push_pop_min_aligned_single(void)
{
    alia_struct_spec spec = alia_stack_object_spec();
    void* obj = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(obj != NULL);

    size_t cap = 64 * 1024;
    void* buf = aligned_alloc_portable(ALIA_MAX_ALIGN, cap);
    TEST_ASSERT(buf != NULL);

    alia_stack* s = alia_stack_init(obj, buf, cap);
    TEST_ASSERT(s != NULL);

    // Push a min-aligned payload (size multiple of ALIA_MIN_ALIGN).
    uint16_t payload_size = 32;
    uint8_t* p = (uint8_t*) alia_stack_push(s, payload_size, &VT_A);
    TEST_ASSERT(p != NULL);
    TEST_ASSERT(is_aligned_ptr(p, ALIA_MIN_ALIGN));

    // Fill payload so we can verify peek_payload points to it.
    for (uint16_t i = 0; i < payload_size; ++i)
        p[i] = (uint8_t) (i ^ 0x5A);

    alia_stack_entry_header const* h = alia_stack_peek_header(s);
    TEST_ASSERT(h != NULL);
    TEST_CHECK(h->vtable == &VT_A);
    TEST_CHECK(h->payload_size == payload_size);
    TEST_CHECK(h->prev_entry_size == 0); // bottom entry
    TEST_CHECK(h->payload_offset >= ALIA_MIN_ALIGN);
    TEST_CHECK(
        (h->payload_offset % ALIA_MIN_ALIGN)
        == 0); // for min-aligned push, should be ALIA_MIN_ALIGN boundary

    void const* peek_p = alia_stack_peek_payload(s);
    TEST_ASSERT(peek_p != NULL);
    TEST_CHECK(peek_p == (void const*) p);

    // Pop makes it empty again.
    alia_stack_pop(s);
    TEST_ASSERT(alia_stack_peek_header(s) == NULL);
    TEST_ASSERT(alia_stack_peek_payload(s) == NULL);

    alia_stack_cleanup(s);
    aligned_free_portable(buf);
    aligned_free_portable(obj);
}

static void
test_stack_push_aligned_and_prev_chain(void)
{
    alia_struct_spec spec = alia_stack_object_spec();
    void* obj = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(obj != NULL);

    size_t cap = 128 * 1024;
    void* buf = aligned_alloc_portable(ALIA_MAX_ALIGN, cap);
    TEST_ASSERT(buf != NULL);

    alia_stack* s = alia_stack_init(obj, buf, cap);
    TEST_ASSERT(s != NULL);

    // Entry 1: min-aligned push (fast path)
    uint16_t sz1 = 16; // multiple of ALIA_MIN_ALIGN
    uint8_t* p1 = (uint8_t*) alia_stack_push(s, sz1, &VT_A);
    TEST_ASSERT(p1 != NULL);
    TEST_CHECK(is_aligned_ptr(p1, ALIA_MIN_ALIGN));
    memset(p1, 0xA1, sz1);

    alia_stack_entry_header const* h1 = alia_stack_peek_header(s);
    TEST_ASSERT(h1 != NULL);
    TEST_CHECK(h1->vtable == &VT_A);
    TEST_CHECK(h1->payload_size == sz1);
    TEST_CHECK(h1->prev_entry_size == 0);
    TEST_CHECK(
        h1->payload_offset
        == (uint16_t) ((uint8_t*) alia_stack_peek_payload(s) - (uint8_t*) h1));

    // Entry 2: custom-aligned push
    uint16_t sz2 = 24;
    uint16_t a2 = 64;
    uint8_t* p2 = (uint8_t*) alia_stack_push_aligned(s, sz2, a2, &VT_B);
    TEST_ASSERT(p2 != NULL);
    TEST_CHECK(is_aligned_ptr(p2, a2));
    memset(p2, 0xB2, sz2);

    alia_stack_entry_header const* h2 = alia_stack_peek_header(s);
    TEST_ASSERT(h2 != NULL);
    TEST_CHECK(h2->vtable == &VT_B);
    TEST_CHECK(h2->payload_size == sz2);
    TEST_CHECK(
        h2->payload_offset
        == (uint16_t) ((uint8_t*) alia_stack_peek_payload(s) - (uint8_t*) h2));

    TEST_CHECK(
        h2->prev_entry_size == ((uint16_t) ((uint8_t*) h2 - (uint8_t*) h1)));

    // Entry 3: another min-aligned push
    uint16_t sz3 = 32;
    uint8_t* p3 = (uint8_t*) alia_stack_push(s, sz3, &VT_A);
    TEST_ASSERT(p3 != NULL);
    TEST_CHECK(is_aligned_ptr(p3, ALIA_MIN_ALIGN));
    memset(p3, 0xC3, sz3);

    alia_stack_entry_header const* h3 = alia_stack_peek_header(s);
    TEST_ASSERT(h3 != NULL);
    TEST_CHECK(h3->vtable == &VT_A);
    TEST_CHECK(h3->payload_size == sz3);
    TEST_CHECK(
        h3->payload_offset
        == (uint16_t) ((uint8_t*) alia_stack_peek_payload(s) - (uint8_t*) h3));
    TEST_CHECK(
        h3->prev_entry_size == (uint16_t) ((uint8_t*) h3 - (uint8_t*) h2));

    // Pop 3 -> top becomes entry 2 again
    alia_stack_pop(s);
    alia_stack_entry_header const* after_pop_h2 = alia_stack_peek_header(s);
    TEST_ASSERT(after_pop_h2 != NULL);
    TEST_CHECK(after_pop_h2->vtable == &VT_B);
    TEST_CHECK(alia_stack_peek_payload(s) == (void const*) p2);
    TEST_CHECK(
        after_pop_h2->prev_entry_size
        == (uint16_t) ((uint8_t*) h2 - (uint8_t*) h1));

    // Pop 2 -> top becomes entry 1 again
    alia_stack_pop(s);
    alia_stack_entry_header const* after_pop_h1 = alia_stack_peek_header(s);
    TEST_ASSERT(after_pop_h1 != NULL);
    TEST_CHECK(after_pop_h1->vtable == &VT_A);
    TEST_CHECK(alia_stack_peek_payload(s) == (void const*) p1);
    TEST_CHECK(after_pop_h1->prev_entry_size == 0);

    // Pop 1 -> empty
    alia_stack_pop(s);
    TEST_ASSERT(alia_stack_peek_header(s) == NULL);

    alia_stack_cleanup(s);
    aligned_free_portable(buf);
    aligned_free_portable(obj);
}

static void
test_stack_reset_discards_entries(void)
{
    alia_struct_spec spec = alia_stack_object_spec();
    void* obj = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(obj != NULL);

    size_t cap = 64 * 1024;
    void* buf = aligned_alloc_portable(ALIA_MAX_ALIGN, cap);
    TEST_ASSERT(buf != NULL);

    alia_stack* s = alia_stack_init(obj, buf, cap);
    TEST_ASSERT(s != NULL);

    // Push a couple entries
    uint8_t* p1 = (uint8_t*) alia_stack_push(s, 16, &VT_A);
    TEST_ASSERT(p1 != NULL);
    uint8_t* p2 = (uint8_t*) alia_stack_push(s, 32, &VT_B);
    TEST_ASSERT(p2 != NULL);

    TEST_ASSERT(alia_stack_peek_header(s) != NULL);

    // Reset should discard all.
    alia_stack_reset(s);
    TEST_ASSERT(alia_stack_peek_header(s) == NULL);
    TEST_ASSERT(alia_stack_peek_payload(s) == NULL);

    // Stack should still be usable.
    uint8_t* p3 = (uint8_t*) alia_stack_push(s, 16, &VT_A);
    TEST_ASSERT(p3 != NULL);
    TEST_ASSERT(alia_stack_peek_header(s) != NULL);
    alia_stack_pop(s);
    TEST_ASSERT(alia_stack_peek_header(s) == NULL);

    alia_stack_cleanup(s);
    aligned_free_portable(buf);
    aligned_free_portable(obj);
}

void
stack_tests(void)
{
    test_stack_lifecycle_reset_empty();
    test_stack_push_pop_min_aligned_single();
    test_stack_push_aligned_and_prev_chain();
    test_stack_reset_discards_entries();
}
