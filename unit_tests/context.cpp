#include <alia/context.hpp>
#include <alia/flow/events.hpp>

#include <catch.hpp>

using namespace alia;

struct other_traversal_tag
{
};
struct other_traversal
{
};

TEST_CASE("component_storage", "[context]")
{
    component_storage storage;

    REQUIRE(!has_component<data_traversal_tag>(storage));
    data_traversal data;
    add_component<data_traversal_tag>(storage, &data);
    REQUIRE(has_component<data_traversal_tag>(storage));
    REQUIRE(get_component<data_traversal_tag>(storage) == &data);
    remove_component<data_traversal_tag>(storage);
    REQUIRE(!has_component<data_traversal_tag>(storage));

    REQUIRE(!has_component<event_traversal_tag>(storage));
    event_traversal event;
    add_component<event_traversal_tag>(storage, &event);
    REQUIRE(has_component<event_traversal_tag>(storage));
    REQUIRE(get_component<event_traversal_tag>(storage) == &event);
    remove_component<event_traversal_tag>(storage);
    REQUIRE(!has_component<event_traversal_tag>(storage));

    REQUIRE(!has_component<other_traversal_tag>(storage));
    other_traversal other;
    add_component<other_traversal_tag>(storage, &other);
    REQUIRE(has_component<other_traversal_tag>(storage));
    REQUIRE(get_component<other_traversal_tag>(storage) == &other);
    remove_component<other_traversal_tag>(storage);
    REQUIRE(!has_component<other_traversal_tag>(storage));
}
