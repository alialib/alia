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

    REQUIRE(!storage.has_component<data_traversal_tag>());
    data_traversal data;
    storage.add_component<data_traversal_tag>(&data);
    REQUIRE(storage.has_component<data_traversal_tag>());
    REQUIRE(storage.get_component<data_traversal_tag>() == &data);
    storage.remove_component<data_traversal_tag>();
    REQUIRE(!storage.has_component<data_traversal_tag>());

    REQUIRE(!storage.has_component<event_traversal_tag>());
    event_traversal event;
    storage.add_component<event_traversal_tag>(&event);
    REQUIRE(storage.has_component<event_traversal_tag>());
    REQUIRE(storage.get_component<event_traversal_tag>() == &event);
    storage.remove_component<event_traversal_tag>();
    REQUIRE(!storage.has_component<event_traversal_tag>());

    REQUIRE(!storage.has_component<other_traversal_tag>());
    other_traversal other;
    storage.add_component<other_traversal_tag>(&other);
    REQUIRE(storage.has_component<other_traversal_tag>());
    REQUIRE(storage.get_component<other_traversal_tag>() == &other);
    storage.remove_component<other_traversal_tag>();
    REQUIRE(!storage.has_component<other_traversal_tag>());
}
