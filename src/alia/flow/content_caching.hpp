#ifndef ALIA_FLOW_CONTENT_CACHING_HPP
#define ALIA_FLOW_CONTENT_CACHING_HPP

#include <alia/context/interface.hpp>
#include <alia/flow/components.hpp>
#include <alia/flow/data_graph.hpp>

namespace alia {

template<class Object, class Content>
auto
implement_alia_content_caching(context, Object&, bool, Content content)
{
    return content;
}

struct component_caching_data
{
    data_block context_setup_block;
    data_block content_block;
    component_container_ptr container;
    captured_id args_id;
};

template<class Context, class Component, class... Args>
void
invoke_pure_component(Context ctx, Component&& component, Args&&... args)
{
    component_caching_data* data;
    if (get_data(ctx, &data))
        data->container.reset(new component_container);

    scoped_component_container container(ctx, &data->container);

    auto invoke_content = [&]() {
        scoped_data_block content_block(ctx, data->content_block);
        component(ctx, std::forward<Args>(args)...);
    };

    if (is_refresh_event(ctx))
    {
        bool content_traversal_required = container.is_dirty();

        auto args_id = combine_ids(ref(args.value_id())...);
        if (!data->args_id.matches(args_id))
            content_traversal_required = true;

        scoped_data_block context_setup(ctx, data->context_setup_block);

        auto invoker = alia::fold_over_collection(
            get_structural_collection(ctx),
            [&](auto, auto& object, auto content) {
                return implement_alia_content_caching(
                    ctx, object, content_traversal_required, content);
            },
            [&]() {
                if (content_traversal_required)
                {
                    invoke_content();
                }
            });
        invoker();

        data->args_id.capture(args_id);
    }
    else
    {
        if (container.is_on_route())
        {
            invoke_content();
        }
    }
}

} // namespace alia

#endif
