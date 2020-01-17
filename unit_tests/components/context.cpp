#include <alia/components/context.hpp>
#include <alia/flow/events.hpp>

#include <catch.hpp>

using namespace alia;

struct other_traversal
{
};
ALIA_DEFINE_COMPONENT_TYPE(other_traversal_tag, other_traversal)

TEST_CASE("context_component_storage", "[context]")
{
    context_component_storage storage;

    REQUIRE(!storage.has<data_traversal_tag>());
    data_traversal data;
    storage.add<data_traversal_tag>(&data);
    REQUIRE(storage.has<data_traversal_tag>());
    REQUIRE(storage.get<data_traversal_tag>() == &data);
    storage.remove<data_traversal_tag>();
    REQUIRE(!storage.has<data_traversal_tag>());

    REQUIRE(!storage.has<event_traversal_tag>());
    event_traversal event;
    storage.add<event_traversal_tag>(&event);
    REQUIRE(storage.has<event_traversal_tag>());
    REQUIRE(storage.get<event_traversal_tag>() == &event);
    storage.remove<event_traversal_tag>();
    REQUIRE(!storage.has<event_traversal_tag>());

    REQUIRE(!storage.has<other_traversal_tag>());
    other_traversal other;
    storage.add<other_traversal_tag>(&other);
    REQUIRE(storage.has<other_traversal_tag>());
    REQUIRE(storage.get<other_traversal_tag>().ptr == &other);
    storage.remove<other_traversal_tag>();
    REQUIRE(!storage.has<other_traversal_tag>());
}
