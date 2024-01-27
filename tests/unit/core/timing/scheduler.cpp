#include <alia/core/timing/scheduler.hpp>

#include <catch2/catch_test_macros.hpp>

using namespace alia;

TEST_CASE("callback_scheduler", "[timing][scheduler]")
{
    component_identity id_a, id_b;
    component_id a, b;
    a = &id_a;
    b = &id_b;

    callback_scheduler scheduler;
    REQUIRE(!has_scheduled_callbacks(scheduler));

    std::ostringstream log;

    auto make_callback = [&](external_component_id component) {
        return [=, &log, &a, &b] {
            bool recognized = component.id == a || component.id == b;
            REQUIRE(recognized);
            REQUIRE(component.identity.use_count() == 0);
            if (component.id == a)
                log << "a:";
            else
                log << "b:";
        };
    };

    auto invoke_callbacks
        = [&](callback_scheduler& scheduler, millisecond_count now) {
              invoke_ready_callbacks(
                  scheduler, now, [&](auto&& callback, auto trigger_time) {
                      callback();
                      log << trigger_time << ";";
                  });
          };

    // Do the most basic case.
    schedule_callback(scheduler, make_callback(externalize(a)), 10);
    REQUIRE(has_scheduled_callbacks(scheduler));
    REQUIRE(time_until_next_callback(scheduler, 4) == 6);
    REQUIRE(time_until_next_callback(scheduler, 12) == 0);
    invoke_callbacks(scheduler, 12);
    REQUIRE(log.str() == "a:10;");
    log.str(std::string());
    REQUIRE(!has_scheduled_callbacks(scheduler));

    // Do some more complicated behavior.
    schedule_callback(scheduler, make_callback(externalize(b)), 20);
    REQUIRE(has_scheduled_callbacks(scheduler));
    REQUIRE(time_until_next_callback(scheduler, 15) == 5);
    schedule_callback(scheduler, make_callback(externalize(a)), 17);
    REQUIRE(has_scheduled_callbacks(scheduler));
    REQUIRE(time_until_next_callback(scheduler, 15) == 2);
    // Add another event for a.
    schedule_callback(scheduler, make_callback(externalize(a)), 15);
    REQUIRE(time_until_next_callback(scheduler, 15) == 0);
    invoke_callbacks(scheduler, 15);
    REQUIRE(log.str() == "a:15;");
    log.str(std::string());
    REQUIRE(has_scheduled_callbacks(scheduler));
    REQUIRE(time_until_next_callback(scheduler, 15) == 2);
    invoke_callbacks(scheduler, 17);
    REQUIRE(log.str() == "a:17;");
    log.str(std::string());
    // Try scheduling another callback during the processing of the original
    // callbacks and make sure it's not processed immediately.
    invoke_ready_callbacks(
        scheduler, 30, [&](auto&& callback, auto trigger_time) {
            callback();
            log << trigger_time << ";";
            schedule_callback(scheduler, make_callback(externalize(b)), 25);
        });
    REQUIRE(log.str() == "b:20;");
    log.str(std::string());
    REQUIRE(has_scheduled_callbacks(scheduler));
    schedule_callback(scheduler, make_callback(externalize(a)), 30);
    invoke_callbacks(scheduler, 30);
    REQUIRE(log.str() == "b:25;a:30;");
    log.str(std::string());
    REQUIRE(!has_scheduled_callbacks(scheduler));
}
