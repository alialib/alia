#ifndef ALIA_SIGNALS_ASYNC_HPP
#define ALIA_SIGNALS_ASYNC_HPP

#include <alia/context/interface.hpp>
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
struct async_operation_data
{
    async_status status = async_status::UNREADY;
    // This is used to identify the result of the operation. It's incremented
    // every time the inputs change.
    counter_type version = 0;
    // If status is COMPLETE, this is the result.
    Value result;
    // If status is FAILED, this is the error.
    std::exception_ptr error;
};

template<class Value>
void
reset(async_operation_data<Value>& data)
{
    ++data.version;
    data.status = async_status::UNREADY;
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
struct async_reporter
{
    void
    report_success(Result result) const
    {
        auto& data = *data_;
        if (data.version == version_)
        {
            data.result = std::move(result);
            data.status = async_status::COMPLETE;
            mark_dirty_component(container_);
            refresh_system(*system_);
        }
    }

    void
    report_failure(std::exception_ptr error) const
    {
        auto& data = *data_;
        if (data.version == version_)
        {
            data.status = async_status::FAILED;
            data.error = error;
            mark_dirty_component(container_);
            refresh_system(*system_);
        }
    }

    std::shared_ptr<async_operation_data<Result>> data_;
    counter_type version_;
    alia::system* system_;
    component_container_ptr container_;
};

template<class Result, class Context, class Launcher, class... Args>
auto
async(Context ctx, Launcher launcher, Args const&... args)
{
    std::shared_ptr<async_operation_data<Result>>& data_ptr
        = get_cached_data<std::shared_ptr<async_operation_data<Result>>>(ctx);
    if (!data_ptr)
        data_ptr.reset(new async_operation_data<Result>);
    auto& data = *data_ptr;

    bool args_ready = true;
    process_async_args(ctx, data, args_ready, args...);

    on_refresh(ctx, [&](auto ctx) {
        if (data.status == async_status::UNREADY && args_ready)
        {
            try
            {
                auto reporter = async_reporter<Result>{
                    data_ptr,
                    data.version,
                    &get<system_tag>(ctx),
                    get_active_component_container(ctx)};
                launcher(ctx, reporter, read_signal(args)...);
                data.status = async_status::LAUNCHED;
            }
            catch (...)
            {
                data.error = std::current_exception();
                data.status = async_status::FAILED;
            }
        }
        if (data.status == async_status::FAILED)
            std::rethrow_exception(data.error);
    });

    return make_async_signal(data);
}

} // namespace alia

#endif
