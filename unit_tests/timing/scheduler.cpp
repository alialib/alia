#include <alia/timing/scheduler.hpp>

#include <testing.hpp>

using namespace alia;

TEST_CASE("timer_event_scheduler", "[timing][scheduler]")
{
    component_identity id_a, id_b;
    routable_component_id a, b;
    a.id = &id_a;
    b.id = &id_b;

    std::ostringstream log;
    auto issuer = [&](routable_component_id component, millisecond_count time) {
        bool recognized = component.id == &id_a || component.id == &id_b;
        REQUIRE(recognized);
        REQUIRE(!component.region);
        if (component.id == &id_a)
            log << "a:";
        else
            log << "b:";
        log << time << ";";
    };

    timer_event_scheduler scheduler;
    REQUIRE(!has_scheduled_events(scheduler));

    // Do the most basic case.
    schedule_event(scheduler, a, 10);
    REQUIRE(has_scheduled_events(scheduler));
    REQUIRE(get_time_until_next_event(scheduler, 4) == 6);
    REQUIRE(get_time_until_next_event(scheduler, 12) == 0);
    issue_ready_events(scheduler, 12, issuer);
    REQUIRE(log.str() == "a:10;");
    log.str(std::string());
    REQUIRE(!has_scheduled_events(scheduler));

    // Do some more complicated behavior.
    schedule_event(scheduler, b, 20);
    REQUIRE(has_scheduled_events(scheduler));
    REQUIRE(get_time_until_next_event(scheduler, 15) == 5);
    schedule_event(scheduler, a, 17);
    REQUIRE(has_scheduled_events(scheduler));
    REQUIRE(get_time_until_next_event(scheduler, 15) == 2);
    // Add another event for a.
    schedule_event(scheduler, a, 15);
    REQUIRE(get_time_until_next_event(scheduler, 15) == 0);
    issue_ready_events(scheduler, 15, issuer);
    REQUIRE(log.str() == "a:15;");
    log.str(std::string());
    REQUIRE(has_scheduled_events(scheduler));
    REQUIRE(get_time_until_next_event(scheduler, 15) == 2);
    issue_ready_events(scheduler, 17, issuer);
    REQUIRE(log.str() == "a:17;");
    log.str(std::string());
    // Try scheduling another event during the processing of the original events
    // and make sure it's not processed immediately.
    issue_ready_events(
        scheduler,
        30,
        [&](routable_component_id component, millisecond_count time) {
            issuer(component, time);
            schedule_event(scheduler, b, 25);
        });
    REQUIRE(log.str() == "b:20;");
    log.str(std::string());
    REQUIRE(has_scheduled_events(scheduler));
    schedule_event(scheduler, a, 30);
    issue_ready_events(scheduler, 30, issuer);
    REQUIRE(log.str() == "b:25;a:30;");
    log.str(std::string());
    REQUIRE(!has_scheduled_events(scheduler));
}
