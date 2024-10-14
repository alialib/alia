#include <alia/core/flow/try_catch.hpp>

#include <alia/core/system/internals.hpp>

namespace alia {

try_block::try_block(core_context ctx) : ctx_(ctx)
{
    get_cached_data(ctx, &data_);
    refresh_handler(ctx, [&](auto ctx) {
        auto refresh_counter = get<core_system_tag>(ctx).refresh_counter;
        if (data_->last_refresh != refresh_counter)
        {
            // This is a new refresh pass, so clear out any exception that we
            // might have stored. We'll (re)try the body content under the
            // assumption that it won't throw an exception this time.
            data_->exception = nullptr;
            data_->last_refresh = refresh_counter;
        }
        else if (data_->exception && data_->uncaught)
        {
            // An exception was thrown from inside this try_block, but none of
            // the associated catch statements could handle it, so we need to
            // rethrow it to the surrounding components.
            std::rethrow_exception(data_->exception);
        }
    });
    uncaught_ = data_->exception ? true : false;
}

void
try_block::operator<<(function_view<void()> body)
{
    // This if statement will evaluate to true if either:
    // 1. We are starting a new refresh. In this case, we need to try the body
    //    to see if it actually throws.
    // 2. We are processing a non-refresh event and there is no exception being
    //    generated from inside the body. In this case, the body is the proper
    //    component to handle the event.
    ALIA_EVENT_DEPENDENT_IF_ (ctx_, !data_->exception)
    {
        try
        {
            body();
        }
        catch (...)
        {
            if (is_refresh_event(ctx_))
            {
                // We tried the body and got an exception, so record it for
                // processing by the catch clauses.
                data_->exception = std::current_exception();
                data_->uncaught = false;
                mark_dirty_component(ctx_);
            }
            else
            {
                // An exception was thrown by an event handler. (This could
                // just be a normal traversal_aborted event.) Just pass it
                // along.
                throw;
            }
        }
    }
    ALIA_END
}

try_block::~try_block()
{
    if (uncaught_ && !exception_detector_.detect())
    {
        // We caught an exception but failed to handle it, so we need to record
        // that fact so we can rethrow it to the surrounding components.
        mark_dirty_component(ctx_);
        data_->uncaught = true;
    }
}

} // namespace alia
