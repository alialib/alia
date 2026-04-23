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

typedef struct test_allocator_state
{
    uint32_t alloc_count;
    uint32_t free_count;
    uint32_t custom_clone_count;
    uint32_t custom_destroy_count;
} test_allocator_state;

static int
is_pow2_u32(uint32_t x)
{
    return x && ((x & (x - 1u)) == 0u);
}

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

static void*
test_alloc(void* user_data, size_t size, size_t alignment)
{
    test_allocator_state* state = (test_allocator_state*) user_data;
    void* p = NULL;
    if (alignment <= sizeof(void*))
        p = malloc(size);
    else if (is_pow2_u32((uint32_t) alignment))
        p = aligned_alloc_portable(alignment, size);

    if (p)
        state->alloc_count++;
    return p;
}

static void
test_free(void* user_data, void* ptr, size_t size, size_t alignment)
{
    (void) size;
    test_allocator_state* state = (test_allocator_state*) user_data;
    if (alignment <= sizeof(void*))
        free(ptr);
    else
        aligned_free_portable(ptr);
    state->free_count++;
}

static alia_general_allocator
make_allocator(test_allocator_state* state)
{
    alia_general_allocator allocator
        = {.alloc = test_alloc, .free = test_free, .user_data = state};
    return allocator;
}

typedef struct custom_payload
{
    uint32_t a;
    uint32_t b;
    uint32_t c;
} custom_payload;

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

static void*
custom_clone(
    alia_general_allocator* allocator, void const* data, uint32_t size)
{
    test_allocator_state* state = (test_allocator_state*) allocator->user_data;
    void* copy = allocator->alloc(allocator->user_data, size, 8u);
    TEST_ASSERT(copy != NULL);
    memcpy(copy, data, size);
    state->custom_clone_count++;
    return copy;
}

static void
custom_destroy(alia_general_allocator* allocator, void* data, uint32_t size)
{
    test_allocator_state* state = (test_allocator_state*) allocator->user_data;
    state->custom_destroy_count++;
    allocator->free(allocator->user_data, data, size, 8u);
}

static alia_id_vtable const custom_vtable
    = {.equals = custom_equals,
       .hash = custom_hash,
       .clone = custom_clone,
       .destroy = custom_destroy};

static void
test_builtin_constructors_and_matchers(void)
{
    int dummy = 0;
    char const small[] = "abc";
    char const large[] = "abcdefghijk";

    alia_id i32 = alia_id_make_i32(-17);
    TEST_CHECK(alia_id_matches_i32(i32, -17));
    TEST_CHECK(!alia_id_matches_i32(i32, -18));

    alia_id u32 = alia_id_make_u32(123u);
    TEST_CHECK(alia_id_matches_u32(u32, 123u));
    TEST_CHECK(!alia_id_matches_u32(u32, 124u));

    alia_id i64 = alia_id_make_i64(-9000000000ll);
    TEST_CHECK(alia_id_matches_i64(i64, -9000000000ll));

    alia_id u64 = alia_id_make_u64(9000000000ull);
    TEST_CHECK(alia_id_matches_u64(u64, 9000000000ull));

    alia_id ptr = alia_id_make_pointer(&dummy);
    TEST_CHECK(alia_id_matches_pointer(ptr, &dummy));

    alia_id bytes_inline = alia_id_make_bytes(small, 3u);
    TEST_CHECK(alia_id_is_inline(bytes_inline));
    TEST_CHECK(alia_id_matches_bytes(bytes_inline, small, 3u));

    alia_id bytes_external = alia_id_make_bytes(large, 11u);
    TEST_CHECK(!alia_id_is_inline(bytes_external));
    TEST_CHECK(alia_id_matches_bytes(bytes_external, large, 11u));
}

static void
test_equality_and_hash_for_builtin_ids(void)
{
    char const value_a[] = "abcdefghijk";
    char const value_b[] = "abcdefghijk";
    char const value_c[] = "abcxefghijk";

    alia_id a = alia_id_make_u64(42u);
    alia_id b = alia_id_make_u64(42u);
    alia_id c = alia_id_make_u64(43u);
    TEST_CHECK(alia_id_equals(a, b));
    TEST_CHECK(!alia_id_equals(a, c));
    TEST_CHECK(alia_id_hash(a) == alia_id_hash(b));

    alia_id bytes1 = alia_id_make_bytes(value_a, 11u);
    alia_id bytes2 = alia_id_make_bytes(value_b, 11u);
    alia_id bytes3 = alia_id_make_bytes(value_c, 11u);
    TEST_CHECK(alia_id_equals(bytes1, bytes2));
    TEST_CHECK(!alia_id_equals(bytes1, bytes3));
    TEST_CHECK(alia_id_hash(bytes1) == alia_id_hash(bytes2));
}

static void
test_composite_equality_and_hash(void)
{
    alia_id a_parts[3]
        = {alia_id_make_i32(1), alia_id_make_u32(2), alia_id_make_u64(3)};
    alia_id b_parts[3]
        = {alia_id_make_i32(1), alia_id_make_u32(2), alia_id_make_u64(3)};
    alia_id c_parts[3]
        = {alia_id_make_u32(2), alia_id_make_i32(1), alia_id_make_u64(3)};

    alia_id a = alia_id_make_composite(a_parts, 3u);
    alia_id b = alia_id_make_composite(b_parts, 3u);
    alia_id c = alia_id_make_composite(c_parts, 3u);

    TEST_CHECK(alia_id_equals(a, b));
    TEST_CHECK(!alia_id_equals(a, c));
    TEST_CHECK(alia_id_hash(a) == alia_id_hash(b));
    TEST_CHECK(alia_id_hash(a) != alia_id_hash(c));
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
        alia_id id = alia_id_make_u32(i);
        uint32_t bucket = alia_id_hash(id) % bucket_count;
        buckets[bucket]++;
    }

    for (uint32_t i = 0; i < bucket_count; ++i)
    {
        if (buckets[i] > 0)
            used++;
        if (buckets[i] > max_bucket)
            max_bucket = buckets[i];
    }

    // This is intentionally loose: we only need basic dispersion quality.
    TEST_CHECK(used >= 48);
    TEST_CHECK(max_bucket <= 24);
}

static void
test_clone_and_destroy_for_bytes_and_composite(void)
{
    test_allocator_state alloc_state;
    memset(&alloc_state, 0, sizeof(alloc_state));
    alia_general_allocator allocator = make_allocator(&alloc_state);

    char const external_bytes[] = "abcdefghijk";
    alia_id transient_bytes = alia_id_make_bytes(external_bytes, 11u);
    alia_id cloned_bytes = alia_id_clone(&allocator, transient_bytes);

    TEST_CHECK(alia_id_equals(transient_bytes, cloned_bytes));
    TEST_CHECK(cloned_bytes.payload.external_data != transient_bytes.payload.external_data);
    TEST_CHECK(alloc_state.alloc_count == 1u);

    alia_id_destroy(&allocator, cloned_bytes);
    TEST_CHECK(alloc_state.free_count == 1u);

    alia_id parts[2]
        = {alia_id_make_i32(7), alia_id_make_bytes(external_bytes, 11u)};
    alia_id transient_composite = alia_id_make_composite(parts, 2u);
    alia_id cloned_composite = alia_id_clone(&allocator, transient_composite);

    TEST_CHECK(alia_id_equals(transient_composite, cloned_composite));
    TEST_CHECK(
        cloned_composite.payload.external_data
        != transient_composite.payload.external_data);

    alia_id const* cloned_parts
        = (alia_id const*) cloned_composite.payload.external_data;
    TEST_CHECK(cloned_parts[1].payload.external_data != parts[1].payload.external_data);
    TEST_CHECK(alloc_state.alloc_count == 3u);

    alia_id_destroy(&allocator, cloned_composite);
    TEST_CHECK(alloc_state.free_count == 3u);
}

static void
test_custom_type_registration_and_lifecycle(void)
{
    static uint32_t custom_type_id = 0;
    test_allocator_state alloc_state;
    memset(&alloc_state, 0, sizeof(alloc_state));
    alia_general_allocator allocator = make_allocator(&alloc_state);

    if (custom_type_id == 0)
    {
        custom_type_id = alia_id_register_type(&custom_vtable);
        TEST_ASSERT(custom_type_id >= ALIA_RESERVED_ID_TYPE_COUNT);
    }

    TEST_CHECK(alia_id_get_vtable(custom_type_id) == &custom_vtable);

    custom_payload p1 = {1u, 2u, 3u};
    custom_payload p2 = {1u, 2u, 3u};
    custom_payload p3 = {1u, 2u, 4u};

    alia_id id1
        = {custom_type_id, sizeof(custom_payload), {.external_data = &p1}};
    alia_id id2
        = {custom_type_id, sizeof(custom_payload), {.external_data = &p2}};
    alia_id id3
        = {custom_type_id, sizeof(custom_payload), {.external_data = &p3}};

    TEST_CHECK(alia_id_equals(id1, id2));
    TEST_CHECK(!alia_id_equals(id1, id3));
    TEST_CHECK(alia_id_hash(id1) == alia_id_hash(id2));

    alia_id cloned = alia_id_clone(&allocator, id1);
    TEST_CHECK(alia_id_equals(id1, cloned));
    TEST_CHECK(cloned.payload.external_data != id1.payload.external_data);
    TEST_CHECK(alloc_state.custom_clone_count == 1u);

    alia_id_destroy(&allocator, cloned);
    TEST_CHECK(alloc_state.custom_destroy_count == 1u);
    TEST_CHECK(alloc_state.alloc_count == 1u);
    TEST_CHECK(alloc_state.free_count == 1u);
}

void
ids_tests(void)
{
    test_builtin_constructors_and_matchers();
    test_equality_and_hash_for_builtin_ids();
    test_composite_equality_and_hash();
    test_hash_bucket_spread_for_sequential_values();
    test_clone_and_destroy_for_bytes_and_composite();
    test_custom_type_registration_and_lifecycle();
}
