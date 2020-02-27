#ifndef ALIA_SIGNALS_ASYNC_HPP
#define ALIA_SIGNALS_ASYNC_HPP

#include <alia/components/context.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/flow/events.hpp>
#include <alia/signals/utilities.hpp>

namespace alia {

enum class async_status
{
    UNREADY,
    LAUNCHED,
    COMPLETE,
    FAILED
};

template<class Value>
struct async_operation_data : node_identity
{
    counter_type version = 0;
    Value result;
    async_status status = async_status::UNREADY;
};

template<class Value>
void
reset(async_operation_data<Value>& data)
{
    if (data.status != async_status::UNREADY)
    {
        ++data.version;
        data.status = async_status::UNREADY;
    }
}

template<class Value>
struct async_signal : signal<async_signal<Value>, Value, read_only_signal>
{
    async_signal(async_operation_data<Value>& data) : data_(&data)
    {
    }
    id_interface const&
    value_id() const
    {
        id_ = make_id(data_->version);
        return id_;
    }
    bool
    has_value() const
    {
        return data_->status == async_status::COMPLETE;
    }
    Value const&
    read() const
    {
        return data_->result;
    }

 private:
    async_operation_data<Value>* data_;
    mutable simple_id<counter_type> id_;
};

template<class Value>
async_signal<Value>
make_async_signal(async_operation_data<Value>& data)
{
    return async_signal<Value>(data);
}

template<class Result>
void
process_async_args(context, async_operation_data<Result>&, bool&)
{
}
template<class Result, class Arg, class... Rest>
void
process_async_args(
    context ctx,
    async_operation_data<Result>& data,
    bool& args_ready,
    Arg const& arg,
    Rest const&... rest)
{
    captured_id* cached_id;
    get_cached_data(ctx, &cached_id);
    if (is_refresh_event(ctx))
    {
        if (!signal_has_value(arg))
        {
            reset(data);
            args_ready = false;
        }
        else if (!cached_id->matches(arg.value_id()))
        {
            reset(data);
            cached_id->capture(arg.value_id());
        }
    }
    process_async_args(ctx, data, args_ready, rest...);
}

template<class Result>
struct async_result_event : targeted_event
{
    Result result;
    unsigned version;
};

template<class Result, class Context, class Launcher, class... Args>
auto
async(Context ctx, Launcher launcher, Args const&... args)
{
    async_operation_data<Result>* data_ptr;
    get_cached_data(ctx, &data_ptr);
    auto& data = *data_ptr;

    auto node_id = make_routable_node_id(ctx, data_ptr);

    bool args_ready = true;
    process_async_args(ctx, data, args_ready, args...);

    ALIA_UNTRACKED_IF(
        is_refresh_event(ctx) && data.status == async_status::UNREADY
        && args_ready)
    {
        auto* system = &get_component<system_tag>(ctx);
        auto version = data.version;
        auto report_result = [=](Result result) {
            async_result_event<Result> event;
            event.result = std::move(result);
            event.version = version;
            dispatch_targeted_event(*system, event, node_id);
            refresh_system(*system);
        };

        try
        {
            launcher(ctx, report_result, read_signal(args)...);
        }
        catch (...)
        {
            data.status = async_status::FAILED;
        }
    }
    ALIA_END

    handle_targeted_event<async_result_event<Result>>(
        ctx, data_ptr, [&](auto ctx, auto& e) {
            if (e.version == data.version)
            {
                data.result = std::move(e.result);
                data.status = async_status::COMPLETE;
            }
        });

    return make_async_signal(data);
}

} // namespace alia

#endif
