#include <alia/flow/try_catch.hpp>

#include <alia/system/internals.hpp>

namespace alia {

try_block::try_block(context ctx) : ctx_(ctx)
{
    get_cached_data(ctx, &data_);
    on_refresh(ctx, [&](auto ctx) {
        auto refresh_counter = get<system_tag>(ctx).refresh_counter;
        if (data_->last_refresh != refresh_counter)
        {
            data_->exception = nullptr;
            data_->last_refresh = refresh_counter;
        }
        else if (data_->exception && data_->uncaught)
        {
            std::rethrow_exception(data_->exception);
        }
    });
    uncaught_ = data_->exception ? true : false;
}

void
try_block::operator<<(function_view<void()> body)
{
    ALIA_EVENT_DEPENDENT_IF_(ctx_, !data_->exception)
    {
        try
        {
            body();
        }
        catch (alia::traversal_abortion&)
        {
            throw;
        }
        catch (...)
        {
            data_->exception = std::current_exception();
            data_->uncaught = false;
            mark_dirty_component(ctx_);
        }
    }
    ALIA_END
}

try_block::~try_block()
{
    if (uncaught_ && !std::uncaught_exception())
    {
        mark_dirty_component(ctx_);
        data_->uncaught = true;
    }
}

} // namespace alia
