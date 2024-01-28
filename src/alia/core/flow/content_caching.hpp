#ifndef ALIA_CORE_FLOW_CONTENT_CACHING_HPP
#define ALIA_CORE_FLOW_CONTENT_CACHING_HPP

#include <alia/core/context/interface.hpp>
#include <alia/core/flow/components.hpp>
#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>

namespace alia {

template<class Object, class Content>
auto
implement_alia_content_caching(core_context, Object&, bool, Content content)
{
    return content;
}

struct component_caching_data
{
    data_block context_setup_block;
    data_block content_block;
    component_container_ptr container;
    captured_id content_id;
    std::exception_ptr exception;
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
        // If the component is explicitly marked as dirty or animating, then we
        // need to visit it regardless.
        bool content_traversal_required
            = container.is_dirty() || container.is_animating();

        // Construct the combined ID of the context and the arguments to this
        // component.
        auto content_id
            = combine_ids(ref(get_content_id(ctx)), ref(args.value_id())...);
        // And check if it still matches.
        if (!data->content_id.matches(content_id))
            content_traversal_required = true;

        // If the component code is generating an exception and we have no
        // reason to revisit it, just rethrow the exception.
        if (!content_traversal_required && data->exception)
        {
            std::rethrow_exception(data->exception);
        }

        // This is a bit convoluted, but here we use a fold over the context
        // objects to construct a function object that will implement content
        // caching for this component. Most context objects don't care about
        // caching, so they'll use the default implementation (which doesn't
        // contribute any code to the function object). Context objects that
        // are managing object trees will insert code to handle that. At the
        // heart of the function object is the code to actually invoke the
        // component (if necessary).
        //
        // I'm sure this could be done more directly with some effort, but it's
        // fine for now.
        //
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
                    data->exception = nullptr;
                    // Capture the ID of the component in this state.
                    // Note that even a captured exception is considered a
                    // "successful" traversal because we know that the
                    // component is currently just generating that exception.
                    data->content_id.capture(content_id);
                    try
                    {
                        invoke_content();
                    }
                    catch (alia::traversal_aborted&)
                    {
                        throw;
                    }
                    catch (...)
                    {
                        data->exception = std::current_exception();
                        throw;
                    }
                }
            });
        // Now execute that function that we just constructed.
        invoker();
    }
    else
    {
        // This is not a refresh event, so all we need to know if whether or
        // not this component is on the route to the event target.
        if (container.is_on_route())
        {
            invoke_content();
        }
    }
}

} // namespace alia

#endif
