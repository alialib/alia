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

    REQUIRE(!has_storage_component<data_traversal_tag>(storage));
    data_traversal data;
    add_storage_component<data_traversal_tag>(storage, &data);
    REQUIRE(has_storage_component<data_traversal_tag>(storage));
    REQUIRE(get_storage_component<data_traversal_tag>(storage) == &data);
    remove_storage_component<data_traversal_tag>(storage);
    REQUIRE(!has_storage_component<data_traversal_tag>(storage));

    REQUIRE(!has_storage_component<event_traversal_tag>(storage));
    event_traversal event;
    add_storage_component<event_traversal_tag>(storage, &event);
    REQUIRE(has_storage_component<event_traversal_tag>(storage));
    REQUIRE(get_storage_component<event_traversal_tag>(storage) == &event);
    remove_storage_component<event_traversal_tag>(storage);
    REQUIRE(!has_storage_component<event_traversal_tag>(storage));

    REQUIRE(!has_storage_component<other_traversal_tag>(storage));
    other_traversal other;
    add_storage_component<other_traversal_tag>(storage, &other);
    REQUIRE(has_storage_component<other_traversal_tag>(storage));
    REQUIRE(get_storage_component<other_traversal_tag>(storage) == &other);
    remove_storage_component<other_traversal_tag>(storage);
    REQUIRE(!has_storage_component<other_traversal_tag>(storage));
}
