#include <alia/components/context.hpp>
#include <alia/flow/events.hpp>

#include <catch.hpp>

using namespace alia;

struct other_traversal
{
};
ALIA_DEFINE_COMPONENT_TYPE(other_traversal_tag, other_traversal*)

TEST_CASE("context_component_storage", "[components][context]")
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

TEST_CASE("context", "[components][context]")
{
    context_component_storage storage;

    data_traversal data;
    storage.add<data_traversal_tag>(&data);

    event_traversal event;
    storage.add<event_traversal_tag>(&event);

    context ctx(&storage);

    REQUIRE(has_component<data_traversal_tag>(ctx));
    REQUIRE(get_component<data_traversal_tag>(ctx) == &data);
    dataless_context dataless = remove_component<data_traversal_tag>(ctx);
    REQUIRE(!has_component<data_traversal_tag>(dataless));

    REQUIRE(has_component<event_traversal_tag>(ctx));
    REQUIRE(get_component<event_traversal_tag>(ctx) == &event);

    other_traversal other;
    auto extended = add_component<other_traversal_tag>(ctx, &other);
    REQUIRE(has_component<other_traversal_tag>(extended));
    REQUIRE(get_component<other_traversal_tag>(extended) == &other);
}
