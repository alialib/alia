#include <alia/abi/base/arena.h>
#include <alia/abi/base/object.h>
#include <alia/abi/context.h>
#include <alia/abi/kernel/effect.h>
#include <alia/impl/events.hpp>

#define TEST_NO_MAIN
#include <doctest/doctest.h>

TEST_CASE("effects_run skips the log when the pass is aborted")
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

    alia_event_traversal events{};
    events.aborted = true;

    alia_effect_log effects{};
    alia_context ctx{};
    ctx.scratch = &scratch;
    ctx.events = &events;
    ctx.effects = &effects;

    int dst = 0;
    int const src = 7;
    alia_defer_write(&ctx, &dst, &src, sizeof(src), "aborted write");
    REQUIRE(ctx.effects->head != nullptr);

    alia_run_effects(&ctx);
    CHECK(dst == 0);
    CHECK(ctx.effects->head != nullptr);

    alia_arena_destroy(arena);
    alia_object_free(storage);
    alia_object_free(buffer);
}
