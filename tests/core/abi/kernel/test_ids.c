#include <alia/abi/kernel/ids.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER)
#include <malloc.h>
#endif

// Acutest main is provided by `tests/core/abi/runner.c`.
#define TEST_NO_MAIN
#include "acutest.h"

static void*
aligned_alloc_portable(size_t align, size_t size)
{
#if defined(_MSC_VER)
    return _aligned_malloc(size, align);
#else
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

typedef struct custom_payload
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
} custom_payload;

static int g_custom_release_count;

static uint32_t
hash_u32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

static bool
custom_equals(void const* a_data, void const* b_data, uint32_t size)
{
    return size == sizeof(custom_payload)
        && memcmp(a_data, b_data, sizeof(custom_payload)) == 0;
}

static uint32_t
custom_hash(void const* data, uint32_t size)
{
    custom_payload const* payload = (custom_payload const*) data;
    uint32_t h = 0x1234abcdu ^ size;
    h = hash_u32(h ^ payload->a);
    h = hash_u32(h ^ payload->b);
    h = hash_u32(h ^ payload->c);
    return h;
}

static void
custom_release(void* payload, uint32_t size)
{
    (void) payload;
    (void) size;
    g_custom_release_count++;
}

static void const*
custom_capture(
    alia_id_capture_context* ctx,
    void const* data,
    uint32_t size)
{
    void* dst = alia_id_capture_reserve(ctx, (size_t) size, 8u);
    if (!alia_id_capture_is_discovery(ctx))
        memcpy(dst, data, (size_t) size);
    return dst;
}

static alia_id_vtable const custom_vtable = {.equals = custom_equals,
    .hash = custom_hash,
    .capture = custom_capture,
    .release = custom_release};

static void const*
padded_capture(
    alia_id_capture_context* ctx,
    void const* data,
    uint32_t size)
{
    size_t const dst_size = (size_t) size + 16u;
    void* dst = alia_id_capture_reserve(ctx, dst_size, 16u);
    if (!alia_id_capture_is_discovery(ctx))
    {
        memcpy(dst, data, (size_t) size);
        memset((uint8_t*) dst + (size_t) size, 0, 16u);
    }
    return dst;
}

static alia_id_vtable const padded_custom_vtable
    = {.equals = custom_equals,
       .hash = custom_hash,
       .capture = padded_capture,
       .release = custom_release};

typedef struct small_custom_payload
{
    uint32_t a;
    uint16_t b;
    uint16_t c;
} small_custom_payload;

static int g_small_custom_release_count;

static bool
small_custom_equals(void const* a_data, void const* b_data, uint32_t size)
{
    return size == sizeof(small_custom_payload)
        && memcmp(a_data, b_data, sizeof(small_custom_payload)) == 0;
}

static uint32_t
small_custom_hash(void const* data, uint32_t size)
{
    small_custom_payload const* payload = (small_custom_payload const*) data;
    uint32_t h = 0x9e3779b9u ^ size;
    h = hash_u32(h ^ payload->a);
    h = hash_u32(h ^ payload->b);
    h = hash_u32(h ^ payload->c);
    return h;
}

static void
small_custom_release(void* payload, uint32_t size)
{
    (void) payload;
    (void) size;
    g_small_custom_release_count++;
}

static void const*
small_custom_capture(
    alia_id_capture_context* ctx,
    void const* data,
    uint32_t size)
{
    void* dst = alia_id_capture_reserve(ctx, (size_t) size, 1u);
    if (!alia_id_capture_is_discovery(ctx))
        memcpy(dst, data, (size_t) size);
    return dst;
}

static alia_id_vtable const small_custom_vtable = {.equals = small_custom_equals,
    .hash = small_custom_hash,
    .capture = small_custom_capture,
    .release = small_custom_release};

static void
test_builtin_constructors_and_matchers(void)
{
    int dummy = 0;
    char const small[] = "abc";
    char const large[] = "abcdefghijk";

    alia_id_view i32 = alia_id_view_make_i32(-17);
    TEST_CHECK(alia_id_view_equals(i32, alia_id_view_make_i32(-17)));
    TEST_CHECK(!alia_id_view_equals(i32, alia_id_view_make_i32(-18)));

    alia_id_view u32 = alia_id_view_make_u32(123u);
    TEST_CHECK(alia_id_view_equals(u32, alia_id_view_make_u32(123u)));
    TEST_CHECK(!alia_id_view_equals(u32, alia_id_view_make_u32(124u)));

    alia_id_view i64 = alia_id_view_make_i64(-9000000000ll);
    TEST_CHECK(alia_id_view_equals(i64, alia_id_view_make_i64(-9000000000ll)));

    alia_id_view u64 = alia_id_view_make_u64(9000000000ull);
    TEST_CHECK(alia_id_view_equals(u64, alia_id_view_make_u64(9000000000ull)));

    alia_id_view ptr = alia_id_view_make_pointer(&dummy);
    TEST_CHECK(alia_id_view_equals(ptr, alia_id_view_make_pointer(&dummy)));
    TEST_CHECK(i32.size_and_flags == 0);
    TEST_CHECK(u64.size_and_flags == 0);
    TEST_CHECK(ptr.size_and_flags == 0);

    alia_id_view bytes_inline = alia_id_view_make_bytes(small, 3u);
    TEST_CHECK((bytes_inline.size_and_flags & ALIA_ID_EXTERNAL_FLAG) == 0);
    TEST_CHECK(alia_id_view_equals(bytes_inline, alia_id_view_make_bytes(small, 3u)));

    alia_id_view bytes_external = alia_id_view_make_bytes(large, 11u);
    TEST_CHECK((bytes_external.size_and_flags & ALIA_ID_EXTERNAL_FLAG) != 0);
    TEST_CHECK(
        alia_id_view_equals(bytes_external, alia_id_view_make_bytes(large, 11u)));
}

static void
test_null_ids(void)
{
    alia_id_view null_view = alia_id_view_null();
    TEST_CHECK(alia_id_view_is_null(null_view));
    TEST_CHECK(null_view.type_id == ALIA_ID_TYPE_NONE);
    TEST_CHECK(null_view.size_and_flags == 0);
    TEST_CHECK(alia_id_view_hash(null_view) == 0u);
    TEST_CHECK(alia_id_view_equals(null_view, alia_id_view_null()));

    alia_captured_id null_cap = alia_captured_id_null();
    TEST_CHECK(alia_captured_id_is_null(&null_cap));
    TEST_CHECK(alia_captured_id_as_view(&null_cap)->type_id == ALIA_ID_TYPE_NONE);

    alia_struct_spec spec = alia_captured_id_spec(null_view);
    TEST_CHECK(spec.size == sizeof(alia_captured_id));
    TEST_CHECK(spec.align == _Alignof(alia_captured_id));

    void* mem = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(mem != NULL);
    alia_captured_id_capture_into(null_view, mem, spec.size);
    alia_captured_id* captured = (alia_captured_id*) mem;
    TEST_CHECK(alia_captured_id_is_null(captured));
    alia_captured_id_release(mem);
    aligned_free_portable(mem);
}

static void
test_equality_and_hash_for_builtin_ids(void)
{
    char const value_a[] = "abcdefghijk";
    char const value_b[] = "abcdefghijk";
    char const value_c[] = "abcxefghijk";

    alia_id_view a = alia_id_view_make_u64(42u);
    alia_id_view b = alia_id_view_make_u64(42u);
    alia_id_view c = alia_id_view_make_u64(43u);
    TEST_CHECK(alia_id_view_equals(a, b));
    TEST_CHECK(!alia_id_view_equals(a, c));
    TEST_CHECK(alia_id_view_hash(a) == alia_id_view_hash(b));

    alia_id_view bytes1 = alia_id_view_make_bytes(value_a, 11u);
    alia_id_view bytes2 = alia_id_view_make_bytes(value_b, 11u);
    alia_id_view bytes3 = alia_id_view_make_bytes(value_c, 11u);
    TEST_CHECK(alia_id_view_equals(bytes1, bytes2));
    TEST_CHECK(!alia_id_view_equals(bytes1, bytes3));
    TEST_CHECK(alia_id_view_hash(bytes1) == alia_id_view_hash(bytes2));
}

static void
test_composite_equality_and_hash(void)
{
    alia_id_view a_parts[3]
        = {alia_id_view_make_i32(1),
           alia_id_view_make_u32(2),
           alia_id_view_make_u64(3)};
    alia_id_view b_parts[3]
        = {alia_id_view_make_i32(1),
           alia_id_view_make_u32(2),
           alia_id_view_make_u64(3)};
    alia_id_view c_parts[3]
        = {alia_id_view_make_u32(2),
           alia_id_view_make_i32(1),
           alia_id_view_make_u64(3)};

    alia_id_view a = alia_id_view_make_composite(a_parts, 3u);
    alia_id_view b = alia_id_view_make_composite(b_parts, 3u);
    alia_id_view c = alia_id_view_make_composite(c_parts, 3u);

    TEST_CHECK(alia_id_view_equals(a, b));
    TEST_CHECK(!alia_id_view_equals(a, c));
    TEST_CHECK(alia_id_view_hash(a) == alia_id_view_hash(b));
    TEST_CHECK(alia_id_view_hash(a) != alia_id_view_hash(c));
}

static void
test_hash_bucket_spread_for_sequential_values(void)
{
    enum
    {
        bucket_count = 64,
        sample_count = 512
    };
    uint16_t buckets[bucket_count];
    uint32_t used = 0;
    uint32_t max_bucket = 0;

    memset(buckets, 0, sizeof(buckets));
    for (uint32_t i = 0; i < sample_count; ++i)
    {
        alia_id_view id = alia_id_view_make_u32(i);
        uint32_t bucket = alia_id_view_hash(id) % bucket_count;
        buckets[bucket]++;
    }

    for (uint32_t i = 0; i < bucket_count; ++i)
    {
        if (buckets[i] > 0)
            used++;
        if (buckets[i] > max_bucket)
            max_bucket = buckets[i];
    }

    TEST_CHECK(used >= 48);
    TEST_CHECK(max_bucket <= 24);
}

static void
test_capture_into_bytes_and_composite(void)
{
    char const external_bytes[] = "abcdefghijk";

    alia_id_view transient_bytes
        = alia_id_view_make_bytes(external_bytes, 11u);
    alia_struct_spec spec = alia_captured_id_spec(transient_bytes);
    TEST_ASSERT(spec.size > 0 && spec.align > 0);
    void* mem = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(mem != NULL);
    alia_captured_id_capture_into(transient_bytes, mem, spec.size);
    alia_captured_id* cap = (alia_captured_id*) mem;
    TEST_CHECK(
        alia_id_view_equals(transient_bytes, *alia_captured_id_as_view(cap)));
    alia_captured_id_release(mem);
    aligned_free_portable(mem);

    alia_id_view parts[2]
        = {alia_id_view_make_i32(7),
           alia_id_view_make_bytes(external_bytes, 11u)};
    alia_id_view transient_composite = alia_id_view_make_composite(parts, 2u);
    spec = alia_captured_id_spec(transient_composite);
    TEST_ASSERT(spec.size > 0);
    mem = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(mem != NULL);
    alia_captured_id_capture_into(transient_composite, mem, spec.size);
    cap = (alia_captured_id*) mem;
    TEST_CHECK(alia_id_view_equals(
        transient_composite, *alia_captured_id_as_view(cap)));
    alia_captured_id_release(mem);
    aligned_free_portable(mem);
}

static void
test_captured_id_matches_view(void)
{
    char const external_bytes[] = "abcdefghijk";
    alia_id_view parts[2]
        = {alia_id_view_make_i32(7),
           alia_id_view_make_bytes(external_bytes, 11u)};
    alia_id_view composite = alia_id_view_make_composite(parts, 2u);

    alia_struct_spec spec = alia_captured_id_spec(composite);
    void* mem = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(mem != NULL);
    alia_captured_id_capture_into(composite, mem, spec.size);
    alia_captured_id* captured = (alia_captured_id*) mem;

    TEST_CHECK(alia_captured_id_matches_view(captured, composite));
    TEST_CHECK(!alia_captured_id_matches_view(captured, alia_id_view_make_i32(7)));

    alia_id_view parts_diff[2]
        = {alia_id_view_make_i32(8),
           alia_id_view_make_bytes(external_bytes, 11u)};
    alia_id_view composite_diff = alia_id_view_make_composite(parts_diff, 2u);
    alia_struct_spec spec3 = alia_captured_id_spec(composite_diff);
    void* mem3 = aligned_alloc_portable(spec3.align, spec3.size);
    TEST_ASSERT(mem3 != NULL);
    alia_captured_id_capture_into(composite_diff, mem3, spec3.size);
    alia_captured_id* captured_diff = (alia_captured_id*) mem3;
    TEST_CHECK(alia_captured_id_matches_view(captured_diff, composite_diff));
    TEST_CHECK(!alia_captured_id_matches_view(captured_diff, composite));

    alia_captured_id null_cap = alia_captured_id_null();
    TEST_CHECK(alia_captured_id_matches_view(&null_cap, alia_id_view_null()));

    alia_captured_id_release(mem3);
    alia_captured_id_release(mem);
    aligned_free_portable(mem3);
    aligned_free_portable(mem);
}

static void
test_custom_type_and_release(void)
{
    static uint32_t custom_type_id = 0;
    g_custom_release_count = 0;

    if (custom_type_id == 0)
    {
        custom_type_id = alia_id_register_type(&custom_vtable);
        TEST_ASSERT(custom_type_id >= ALIA_RESERVED_ID_TYPE_COUNT);
    }

    TEST_CHECK(alia_id_get_vtable(custom_type_id) == &custom_vtable);

    custom_payload p1 = {1u, 2u, 3u};
    custom_payload p2 = {1u, 2u, 3u};
    custom_payload p3 = {1u, 2u, 4u};

    alia_id_view id1 = alia_id_view_make_custom_external(
        custom_type_id, &p1, sizeof(custom_payload));
    alia_id_view id2 = alia_id_view_make_custom_external(
        custom_type_id, &p2, sizeof(custom_payload));
    alia_id_view id3 = alia_id_view_make_custom_external(
        custom_type_id, &p3, sizeof(custom_payload));

    TEST_CHECK(alia_id_view_equals(id1, id2));
    TEST_CHECK(!alia_id_view_equals(id1, id3));
    TEST_CHECK(alia_id_view_hash(id1) == alia_id_view_hash(id2));

    alia_struct_spec spec = alia_captured_id_spec(id1);
    TEST_ASSERT(spec.size > 0);
    void* mem = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(mem != NULL);
    alia_captured_id_capture_into(id1, mem, spec.size);
    alia_captured_id* cap = (alia_captured_id*) mem;
    TEST_CHECK(alia_id_view_equals(id1, *alia_captured_id_as_view(cap)));

    TEST_CHECK(g_custom_release_count == 0);
    alia_captured_id_release(mem);
    TEST_CHECK(g_custom_release_count == 1);

    aligned_free_portable(mem);
}

static void
test_custom_inline_constructor(void)
{
    static uint32_t small_custom_type_id = 0;
    g_small_custom_release_count = 0;

    if (small_custom_type_id == 0)
    {
        small_custom_type_id = alia_id_register_type(&small_custom_vtable);
        TEST_ASSERT(small_custom_type_id >= ALIA_RESERVED_ID_TYPE_COUNT);
    }

    small_custom_payload p1 = {1u, 2u, 3u};
    small_custom_payload p2 = {1u, 2u, 3u};
    small_custom_payload p3 = {1u, 2u, 4u};

    alia_id_view id1 = alia_id_view_make_custom_inline(
        small_custom_type_id, &p1, sizeof(small_custom_payload));
    alia_id_view id2 = alia_id_view_make_custom_inline(
        small_custom_type_id, &p2, sizeof(small_custom_payload));
    alia_id_view id3 = alia_id_view_make_custom_inline(
        small_custom_type_id, &p3, sizeof(small_custom_payload));

    TEST_CHECK(alia_id_view_equals(id1, id2));
    TEST_CHECK(!alia_id_view_equals(id1, id3));
    TEST_CHECK(alia_id_view_hash(id1) == alia_id_view_hash(id2));

    alia_struct_spec spec = alia_captured_id_spec(id1);
    TEST_ASSERT(spec.size >= sizeof(alia_captured_id));
    void* mem = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(mem != NULL);
    alia_captured_id_capture_into(id1, mem, spec.size);
    alia_captured_id* cap = (alia_captured_id*) mem;
    TEST_CHECK(alia_captured_id_matches_view(cap, id1));

    TEST_CHECK(g_small_custom_release_count == 0);
    alia_captured_id_release(mem);
    TEST_CHECK(g_small_custom_release_count == 1);
    aligned_free_portable(mem);
}

static void
test_custom_measure_capture_tail_larger_than_view_size(void)
{
    static uint32_t padded_type_id = 0, custom_type_id = 0;
    g_custom_release_count = 0;

    if (padded_type_id == 0)
    {
        padded_type_id = alia_id_register_type(&padded_custom_vtable);
        TEST_ASSERT(padded_type_id >= ALIA_RESERVED_ID_TYPE_COUNT);
    }
    if (custom_type_id == 0)
    {
        custom_type_id = alia_id_register_type(&custom_vtable);
        TEST_ASSERT(custom_type_id >= ALIA_RESERVED_ID_TYPE_COUNT);
    }

    custom_payload p = {9u, 8u, 7u};
    alia_id_view transient = alia_id_view_make_custom_external(
        padded_type_id, &p, sizeof(custom_payload));

    alia_struct_spec spec = alia_captured_id_spec(transient);
    TEST_ASSERT(spec.size > 0);

    alia_id_view baseline = alia_id_view_make_custom_external(
        custom_type_id, &p, sizeof(custom_payload));
    alia_struct_spec baseline_spec = alia_captured_id_spec(baseline);
    TEST_ASSERT(baseline_spec.size > 0);
    TEST_CHECK(spec.size > baseline_spec.size);

    void* mem = aligned_alloc_portable(spec.align, spec.size);
    TEST_ASSERT(mem != NULL);
    alia_captured_id_capture_into(transient, mem, spec.size);
    alia_captured_id* cap = (alia_captured_id*) mem;
    TEST_CHECK(alia_id_view_equals(transient, *alia_captured_id_as_view(cap)));

    alia_captured_id_release(mem);
    TEST_CHECK(g_custom_release_count == 1);
    aligned_free_portable(mem);
}

void
ids_tests(void)
{
    test_null_ids();
    test_builtin_constructors_and_matchers();
    test_equality_and_hash_for_builtin_ids();
    test_composite_equality_and_hash();
    test_hash_bucket_spread_for_sequential_values();
    test_capture_into_bytes_and_composite();
    test_captured_id_matches_view();
    test_custom_type_and_release();
    test_custom_inline_constructor();
    test_custom_measure_capture_tail_larger_than_view_size();
}
