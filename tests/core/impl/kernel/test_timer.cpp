#include <doctest/doctest.h>

#include <alia/abi/kernel/events.h>
#include <alia/abi/kernel/timing.h>
#include <alia/impl/events.hpp>
#include <alia/ui/system/object.h>
#include <alia/ui/system/timer_internal.h>

#include <cstdint>
#include <vector>

using namespace alia;

namespace {

using timer_request = alia_ui_timer_request;
using timer_pq = std::priority_queue<
    timer_request,
    std::vector<timer_request>,
    alia_ui_timer_request_compare>;

} // namespace

TEST_CASE("timer request queue emits in fire_time order")
{
    timer_pq pq;

    alia_element_id t1 = reinterpret_cast<alia_element_id>(1u);
    alia_element_id t2 = reinterpret_cast<alia_element_id>(2u);

    pq.push(
        timer_request{.target = t1, .fire_time = 10, .queued_in_cycle = 0});
    pq.push(timer_request{.target = t2, .fire_time = 5, .queued_in_cycle = 0});
    pq.push(
        timer_request{.target = t1, .fire_time = 20, .queued_in_cycle = 0});

    std::vector<alia_nanosecond_count> emitted;
    alia::process_due_timers_internal(
        pq,
        /*now*/ 100,
        /*cycle*/ 1,
        [&](timer_request const& req) { emitted.push_back(req.fire_time); });

    REQUIRE(emitted.size() == 3);
    CHECK(emitted[0] == 5);
    CHECK(emitted[1] == 10);
    CHECK(emitted[2] == 20);
}

TEST_CASE("timer requests queued in current cycle are deferred")
{
    timer_pq pq;

    uint64_t cycle = 7;
    alia_element_id t1 = reinterpret_cast<alia_element_id>(1u);
    alia_element_id t2 = reinterpret_cast<alia_element_id>(2u);

    // Both are due (<= now), but only one is eligible for dispatch.
    pq.push(
        timer_request{.target = t1, .fire_time = 5, .queued_in_cycle = cycle});
    pq.push(
        timer_request{
            .target = t2, .fire_time = 7, .queued_in_cycle = cycle - 1});

    std::vector<alia_nanosecond_count> emitted;
    alia::process_due_timers_internal(
        pq,
        /*now*/ 10,
        /*cycle*/ cycle,
        [&](timer_request const& req) { emitted.push_back(req.fire_time); });

    REQUIRE(emitted.size() == 1);
    CHECK(emitted[0] == 7);

    // On the next cycle, the deferred one should fire.
    emitted.clear();
    alia::process_due_timers_internal(
        pq,
        /*now*/ 10,
        /*cycle*/ cycle + 1,
        [&](timer_request const& req) { emitted.push_back(req.fire_time); });
    REQUIRE(emitted.size() == 1);
    CHECK(emitted[0] == 5);
}

TEST_CASE("alia_timer_handle_event ignores stale fire_time / target")
{
    // Create an "in component" timer state.
    alia_timer_state state{
        .active = true,
        .expected_fire_time = 100,
        .target = reinterpret_cast<alia_element_id>(0x1234u)};

    // Build a minimal event traversal + context.
    alia_event_traversal traversal{};
    alia_context ctx{};
    ctx.events = &traversal;

    // Fire with wrong time.
    {
        alia_timer payload{.target = state.target, .fire_time = 99};
        alia_event event = alia_make_timer_event(payload);
        traversal.event = &event;

        CHECK(alia_timer_handle_event(&ctx, &state) == false);
        CHECK(state.active == true);
    }

    // Fire with wrong target.
    {
        alia_timer payload{
            .target = reinterpret_cast<alia_element_id>(0xdeadbeefu),
            .fire_time = 100};
        alia_event event = alia_make_timer_event(payload);
        traversal.event = &event;

        CHECK(alia_timer_handle_event(&ctx, &state) == false);
        CHECK(state.active == true);
    }

    // Correct fire event.
    {
        alia_timer payload{.target = state.target, .fire_time = 100};
        alia_event event = alia_make_timer_event(payload);
        traversal.event = &event;

        CHECK(alia_timer_handle_event(&ctx, &state) == true);
        CHECK(state.active == false);
    }
}
