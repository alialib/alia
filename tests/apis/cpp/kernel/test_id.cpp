#include <alia/kernel/id.hpp>

#include <doctest/doctest.h>

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

using namespace alia;
using namespace alia::operators;

TEST_CASE("null id")
{
    CHECK(alia_id_view_is_null(null_id()));
    CHECK((null_id() == null_id()));
}

TEST_CASE("unit id")
{
    CHECK(alia_id_view_is_unit(unit_id()));
    CHECK_FALSE(alia_id_view_is_null(unit_id()));
    CHECK((unit_id() == unit_id()));
    CHECK((unit_id() == alia_id_view_unit()));
    CHECK((unit_id() != null_id()));
}

TEST_CASE("make_id bool and integrals")
{
    CHECK((make_id(false) == alia_id_view_make_u32(0)));
    CHECK((make_id(true) == alia_id_view_make_u32(1)));

    CHECK((make_id(int32_t{-7}) == alia_id_view_make_i32(-7)));
    CHECK((make_id(uint32_t{42}) == alia_id_view_make_u32(42)));
    CHECK((make_id(int64_t{-9}) == alia_id_view_make_i64(-9)));
    CHECK((make_id(uint64_t{99}) == alia_id_view_make_u64(99)));

    CHECK((make_id(1) != make_id(2)));
    CHECK((make_id(true) != make_id(false)));
}

TEST_CASE("make_id float and double")
{
    float const a = 1.5f;
    float const b = 1.5f;
    float const c = 2.5f;
    CHECK((make_id(a) == make_id(b)));
    CHECK((make_id(a) != make_id(c)));

    double const d = 3.25;
    double const e = 3.25;
    CHECK((make_id(d) == make_id(e)));
    CHECK((make_id(d) != make_id(4.0)));
}

TEST_CASE("make_pointer_id")
{
    static char const lit_a[] = "hello";
    static char const lit_b[] = "hello";
    CHECK((make_pointer_id(lit_a) == alia_id_view_make_pointer(lit_a)));
    // Distinct static arrays are distinct pointer identities even with the
    // same contents.
    CHECK((make_pointer_id(lit_a) != make_pointer_id(lit_b)));
}

TEST_CASE("make_id_by_reference")
{
    int const x = 7;
    CHECK((make_id_by_reference(x) == make_id(7)));

    std::string const s = "abc";
    CHECK((make_id_by_reference(s) == alia_id_view_make_bytes("abc", 3u)));

    struct pod
    {
        int a;
        int b;
    };
    pod const p{1, 2};
    CHECK(
        (make_id_by_reference(p)
         == alia_id_view_make_bytes(
             reinterpret_cast<char const*>(&p), sizeof(pod))));
}

TEST_CASE("identifiable")
{
    CHECK((identifiable<bool>));
    CHECK((identifiable<int>));
    CHECK((identifiable<float>));
    CHECK((identifiable<double>));
    CHECK((identifiable<std::string>));

    struct pod
    {
        int a;
        int b;
    };
    CHECK((identifiable<pod>));

    struct no_id
    {
        std::string s;
    };
    CHECK_FALSE((identifiable<no_id>));
    CHECK_FALSE((identifiable<std::vector<int>>));
}

TEST_CASE("std::hash matches alia_id_view_hash")
{
    std::hash<id_view> const hash{};
    CHECK(hash(make_id(7)) == alia_id_view_hash(make_id(7)));
    CHECK(hash(make_id(7)) == hash(make_id(7)));
    CHECK(hash(make_id(7)) != hash(null_id()));
}

TEST_CASE("make_id_pair")
{
    alia_id_pair a_storage{};
    alia_id_pair a_again_storage{};
    alia_id_pair b_storage{};
    auto a = make_id_pair(a_storage, make_id(0), make_id(1));
    auto b = make_id_pair(b_storage, make_id(1), make_id(2));
    CHECK((a != b));
    CHECK((a == make_id_pair(a_again_storage, make_id(0), make_id(1))));
}

namespace {

void*
test_aligned_alloc(size_t size, size_t alignment)
{
#if defined(_MSC_VER)
    return _aligned_malloc(size, alignment);
#else
    size_t const align = alignment < sizeof(void*) ? sizeof(void*) : alignment;
    void* p = nullptr;
    if (posix_memalign(&p, align, size) != 0)
        return nullptr;
    return p;
#endif
}

void
test_aligned_free(void* ptr)
{
#if defined(_MSC_VER)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

alia_general_allocator
test_malloc_allocator()
{
    return alia_general_allocator{
        [](void*, size_t size, size_t alignment) -> void* {
            if (alignment <= sizeof(void*))
                return malloc(size);
            return test_aligned_alloc(size, alignment);
        },
        [](void*, void* ptr, size_t, size_t alignment) {
            if (alignment <= sizeof(void*))
                free(ptr);
            else
                test_aligned_free(ptr);
        },
        nullptr};
}

} // namespace

TEST_CASE("captured_id value types")
{
    captured_id<int> c;
    CHECK(c.empty());
    CHECK_FALSE(c.matches(0));

    c.capture(7);
    CHECK_FALSE(c.empty());
    CHECK(c.matches(7));
    CHECK_FALSE(c.matches(8));

    c.clear();
    CHECK(c.empty());
    CHECK_FALSE(c.matches(7));
}

TEST_CASE("captured_id constant_value_tag")
{
    captured_id<constant_value_tag> c;
    CHECK(c.empty());
    c.capture(constant_value_tag{});
    CHECK(c.matches(constant_value_tag{}));
    c.clear();
    CHECK(c.empty());
}

TEST_CASE("captured_id pair")
{
    captured_id<std::pair<int, int>> c;
    CHECK(c.empty());
    c.capture({1, 2});
    CHECK(c.matches({1, 2}));
    CHECK_FALSE(c.matches({1, 3}));
    c.clear();
    CHECK(c.empty());
}

TEST_CASE("captured_id id_view inline avoids allocation")
{
    int allocs = 0;
    alia_general_allocator alloc = test_malloc_allocator();
    auto counting = alloc;
    counting.user_data = &allocs;
    counting.alloc = [](void* user_data, size_t size, size_t alignment) -> void* {
        ++*static_cast<int*>(user_data);
        return test_malloc_allocator().alloc(nullptr, size, alignment);
    };

    captured_id<id_view> c;
    c.capture(make_id(7), &counting);
    CHECK_FALSE(c.empty());
    CHECK(c.matches(make_id(7)));
    CHECK_FALSE(c.matches(make_id(8)));
    CHECK(allocs == 0);

    // Allocator may be omitted for fully inline IDs.
    c.capture(make_id(9), nullptr);
    CHECK(c.matches(make_id(9)));
    CHECK(allocs == 0);

    c.clear();
    CHECK(c.empty());
}

TEST_CASE("captured_id id_view with external payload")
{
    int allocs = 0;
    alia_general_allocator base = test_malloc_allocator();
    alia_general_allocator alloc = base;
    alloc.user_data = &allocs;
    alloc.alloc = [](void* user_data, size_t size, size_t alignment) -> void* {
        ++*static_cast<int*>(user_data);
        return test_malloc_allocator().alloc(nullptr, size, alignment);
    };
    alloc.free = [](void*, void* ptr, size_t size, size_t alignment) {
        test_malloc_allocator().free(nullptr, ptr, size, alignment);
    };

    captured_id<id_view> c;
    CHECK(c.empty());
    CHECK(c.matches(null_id()));

    std::string text = "hello-captured-id";
    id_view transient = make_id_by_reference(text);
    c.capture(transient, &alloc);
    CHECK(allocs == 1);
    CHECK_FALSE(c.empty());
    CHECK(c.matches(transient));
    CHECK(c.matches(make_id_by_reference(std::string("hello-captured-id"))));
    CHECK_FALSE(c.matches(make_id_by_reference(std::string("other"))));

    // Mutating the original buffer must not change the captured ID.
    text[0] = 'H';
    CHECK(c.matches(make_id_by_reference(std::string("hello-captured-id"))));
    CHECK_FALSE(c.matches(make_id_by_reference(text)));

    c.clear();
    CHECK(c.empty());
    CHECK(c.matches(null_id()));
}
