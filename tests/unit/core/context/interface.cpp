#include <alia/core/context/interface.hpp>

#include <alia/core/flow/events.hpp>
#include <alia/core/system/internals.hpp>

#include <catch2/catch_test_macros.hpp>

#include "traversal.hpp"

using namespace alia;

struct other_traversal
{
};
ALIA_DEFINE_TAGGED_TYPE(other_traversal_tag, other_traversal&)

ALIA_DEFINE_TAGGED_TYPE(by_value_string_tag, std::string)

TEST_CASE("context_storage", "[context][interface]")
{
    core_context_storage storage;

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
    REQUIRE(
        &detail::tagged_data_caster<std::any&, other_traversal&>::apply(
            storage.get<other_traversal_tag>())
        == &other);
    storage.remove<other_traversal_tag>();
    REQUIRE(!storage.has<other_traversal_tag>());
}

TEST_CASE("context", "[context][interface]")
{
    core_context_storage storage;

    alia::test_system sys;
    data_traversal data;
    event_traversal event;
    timing_subsystem timing;

    scoped_data_traversal sdt(sys.data, data);

    core_context ctx = make_context(&storage, sys, event, data, timing);

    REQUIRE(get_content_id(ctx) == unit_id);

    REQUIRE(detail::has_context_object<data_traversal_tag>(ctx));
    REQUIRE(&get<data_traversal_tag>(ctx) == &data);
    dataless_core_context dataless
        = detail::remove_context_object<data_traversal_tag>(ctx);
    REQUIRE(!detail::has_context_object<data_traversal_tag>(dataless));

    REQUIRE(detail::has_context_object<event_traversal_tag>(ctx));
    REQUIRE(&get<event_traversal_tag>(ctx) == &event);

    other_traversal other;
    extend_context_type_t<core_context, other_traversal_tag> extended
        = add_context_object<other_traversal_tag>(ctx, other);
    REQUIRE(detail::has_context_object<data_traversal_tag>(extended));
    REQUIRE(&get<data_traversal_tag>(extended) == &data);
    REQUIRE(detail::has_context_object<event_traversal_tag>(extended));
    REQUIRE(&get<event_traversal_tag>(extended) == &event);
    REQUIRE(detail::has_context_object<other_traversal_tag>(extended));
    REQUIRE(&get<other_traversal_tag>(extended) == &other);

    with_extended_context<other_traversal_tag>(ctx, other, [&](auto with_ctx) {
        REQUIRE(detail::has_context_object<event_traversal_tag>(with_ctx));
        REQUIRE(&get<event_traversal_tag>(with_ctx) == &event);
        REQUIRE(detail::has_context_object<other_traversal_tag>(with_ctx));
        REQUIRE(&get<other_traversal_tag>(with_ctx) == &other);
    });

    auto more_extended
        = add_context_object<by_value_string_tag>(extended, "test");
    REQUIRE(detail::has_context_object<by_value_string_tag>(more_extended));
    REQUIRE(get<by_value_string_tag>(more_extended) == "test");

    with_extended_context<by_value_string_tag>(
        extended, "with", [&](auto with_ctx) {
            REQUIRE(detail::has_context_object<by_value_string_tag>(with_ctx));
            REQUIRE(get<by_value_string_tag>(with_ctx) == "with");
        });
}

TEST_CASE("optional_context", "[context][interface]")
{
    core_context_storage storage;

    alia::test_system sys;
    data_traversal data;
    event_traversal event;
    timing_subsystem timing;

    scoped_data_traversal sdt(sys.data, data);

    core_context ctx = make_context(&storage, sys, event, data, timing);

    optional_context<core_context> optional;
    REQUIRE(!optional);
    optional.reset(ctx);
    REQUIRE(optional);

    REQUIRE(detail::has_context_object<data_traversal_tag>(*optional));
    REQUIRE(&get<data_traversal_tag>(*optional) == &data);
    dataless_core_context dataless
        = detail::remove_context_object<data_traversal_tag>(*optional);
    REQUIRE(!detail::has_context_object<data_traversal_tag>(dataless));

    optional.reset();
    REQUIRE(!optional);
}

TEST_CASE("content IDs", "[context][interface]")
{
    alia::test_system sys;
    initialize_test_system(sys, [](core_context) {});

    captured_id outer_id, inner_id, downcast_id, recalculated_id;

    ALIA_DEFINE_TAGGED_TYPE(outer_tag, value_signal<int>&)
    ALIA_DEFINE_TAGGED_TYPE(inner_tag, value_signal<int>&)

    auto make_controller = [&](int outer, int inner) {
        return [&, outer, inner](core_context root_ctx) {
            REQUIRE(get_content_id(root_ctx) == unit_id);
            auto outer_signal = value(outer);
            with_extended_context<outer_tag>(
                root_ctx, outer_signal, [&](auto outer_ctx) {
                    // To avoid warnings about unused typedefs.
                    REQUIRE(signal_has_value(get<outer_tag>(outer_ctx)));
                    outer_id.capture(get_content_id(outer_ctx));
                    {
                        auto inner_signal = value(inner);
                        with_extended_context<outer_tag>(
                            outer_ctx, inner_signal, [&](auto inner_ctx) {
                                // To avoid warnings about unused typedefs.
                                REQUIRE(signal_has_value(
                                    get<inner_tag>(inner_ctx)));
                                inner_id.capture(get_content_id(inner_ctx));
                                {
                                    decltype(outer_ctx) downcast_ctx
                                        = inner_ctx;
                                    downcast_id.capture(
                                        get_content_id(downcast_ctx));
                                    recalculate_content_id(downcast_ctx);
                                    recalculated_id.capture(
                                        get_content_id(downcast_ctx));
                                }
                            });
                    }
                });
        };
    };

    do_traversal(sys, make_controller(4, 1));
    REQUIRE(downcast_id == inner_id);
    REQUIRE(recalculated_id == outer_id);
    captured_id last_outer = outer_id;
    captured_id last_inner = inner_id;

    do_traversal(sys, make_controller(4, 2));
    REQUIRE(downcast_id == inner_id);
    REQUIRE(recalculated_id == outer_id);
    REQUIRE(last_outer == outer_id);
    REQUIRE(last_inner != inner_id);
    last_inner = inner_id;

    do_traversal(sys, make_controller(4, 2));
    REQUIRE(downcast_id == inner_id);
    REQUIRE(recalculated_id == outer_id);
    REQUIRE(last_outer == outer_id);
    REQUIRE(last_inner == inner_id);

    do_traversal(sys, make_controller(7, 2));
    REQUIRE(last_outer != outer_id);
    last_outer = outer_id;
    REQUIRE(last_inner != inner_id);
    last_inner = inner_id;

    do_traversal(sys, make_controller(7, 2));
    REQUIRE(last_outer == outer_id);
    REQUIRE(last_inner == inner_id);
}
