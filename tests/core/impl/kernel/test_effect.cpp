#include <alia/abi/base/arena.h>
#include <alia/abi/base/object.h>
#include <alia/abi/context.h>
#include <alia/abi/kernel/effect.h>

#define TEST_NO_MAIN
#include <doctest/doctest.h>

TEST_CASE("alia_run_effects applies deferred writes in FIFO order")
{
    alia_struct_spec const arena_spec = alia_arena_object_spec();
    void* storage = alia_object_alloc(arena_spec);
    void* buffer = alia_object_alloc(alia_struct_spec{4096, ALIA_MAX_ALIGN});
    REQUIRE(storage != nullptr);
    REQUIRE(buffer != nullptr);

    alia_arena* arena
        = alia_arena_init(storage, buffer, 4096, alia_arena_no_controller());
    REQUIRE(arena != nullptr);

    alia_bump_allocator scratch;
    alia_bump_allocator_init(&scratch, arena);

    alia_effect_log effects{};
    alia_context ctx{};
    ctx.scratch = &scratch;
    ctx.effects = &effects;

    int dst = 0;
    int const first = 3;
    int const second = 9;
    alia_defer_write(&ctx, &dst, &first, sizeof(first), "first");
    alia_defer_write(&ctx, &dst, &second, sizeof(second), "second");
    REQUIRE(ctx.effects->head != nullptr);
    CHECK(dst == 0);

    alia_run_effects(&ctx);
    CHECK(dst == 9);
    CHECK(ctx.effects->head == nullptr);
    CHECK(ctx.effects->tail == nullptr);

    alia_arena_destroy(arena);
    alia_object_free(storage);
    alia_object_free(buffer);
}
