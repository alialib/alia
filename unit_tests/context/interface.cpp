#include <alia/context/interface.hpp>

#include <alia/flow/events.hpp>
#include <alia/system.hpp>

#include <testing.hpp>

using namespace alia;

struct other_traversal
{
};
ALIA_DEFINE_TAGGED_TYPE(other_traversal_tag, other_traversal&)

TEST_CASE("context_storage", "[context][interface]")
{
    context_storage storage;

    REQUIRE(!storage.has<data_traversal_tag>());
    data_traversal data;
    storage.add<data_traversal_tag>(data);
    REQUIRE(storage.has<data_traversal_tag>());
    REQUIRE(&storage.get<data_traversal_tag>() == &data);
    storage.remove<data_traversal_tag>();
    REQUIRE(!storage.has<data_traversal_tag>());

    REQUIRE(!storage.has<event_traversal_tag>());
    event_traversal event;
    storage.add<event_traversal_tag>(event);
    REQUIRE(storage.has<event_traversal_tag>());
    REQUIRE(&storage.get<event_traversal_tag>() == &event);
    storage.remove<event_traversal_tag>();
    REQUIRE(!storage.has<event_traversal_tag>());

    REQUIRE(!storage.has<other_traversal_tag>());
    other_traversal other;
    storage.add<other_traversal_tag>(std::ref(other));
    REQUIRE(storage.has<other_traversal_tag>());
    REQUIRE(storage.get<other_traversal_tag>().ptr == &other);
    storage.remove<other_traversal_tag>();
    REQUIRE(!storage.has<other_traversal_tag>());
}

TEST_CASE("context", "[context][interface]")
{
    context_storage storage;

    alia::system sys;
    data_traversal data;
    event_traversal event;
    timing_component timing;

    scoped_data_traversal sdt(sys.data, data);

    context ctx = make_context(&storage, sys, event, data, timing);

    REQUIRE(ctx.has<data_traversal_tag>());
    REQUIRE(&ctx.get<data_traversal_tag>() == &data);
    dataless_context dataless = ctx.remove<data_traversal_tag>();
    REQUIRE(!dataless.has<data_traversal_tag>());

    REQUIRE(ctx.has<event_traversal_tag>());
    REQUIRE(&ctx.get<event_traversal_tag>() == &event);

    other_traversal other;
    extend_context_type_t<context, other_traversal_tag> extended
        = copy_context(ctx).extend<other_traversal_tag>(other);
    REQUIRE(extended.has<data_traversal_tag>());
    REQUIRE(&extended.get<data_traversal_tag>() == &data);
    REQUIRE(extended.has<event_traversal_tag>());
    REQUIRE(&extended.get<event_traversal_tag>() == &event);
    REQUIRE(extended.has<other_traversal_tag>());
    REQUIRE(&extended.get<other_traversal_tag>() == &other);
}
