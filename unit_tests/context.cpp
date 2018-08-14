#include <alia/context.hpp>

#include <catch.hpp>

using namespace alia;

TEST_CASE("component_storage", "[context]")
{
    component_storage storage;

    REQUIRE(!has_component<data_traversal_tag>(storage));
    data_traversal traversal;
    add_component<data_traversal_tag>(storage, &traversal);
    REQUIRE(has_component<data_traversal_tag>(storage));
    REQUIRE(get_component<data_traversal_tag>(storage) == &traversal);
    remove_component<data_traversal_tag>(storage);
    REQUIRE(!has_component<data_traversal_tag>(storage));
}
