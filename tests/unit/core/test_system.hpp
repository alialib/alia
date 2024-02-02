#ifndef ALIA_CORE_UNIT_TESTS_SYSTEM_HPP
#define ALIA_CORE_UNIT_TESTS_SYSTEM_HPP

#include <alia/core/system/internals.hpp>
#include <alia/core/timing/ticks.hpp>

namespace alia { namespace {

struct test_system : typed_system<core_context>
{
    std::function<void(core_context)> controller;

    virtual void
    invoke_controller(core_context ctx)
    {
        controller(ctx);
    }
};

void
initialize_test_system(
    test_system& sys,
    std::function<void(core_context)> const& controller,
    external_interface* external = nullptr)
{
    initialize_core_system(sys, external);
    sys.controller = controller;
}

void
process_internal_callbacks(
    test_system& sys,
    millisecond_count now,
    function_view<void(std::function<void()> const&, millisecond_count)> const&
        invoker)
{
    invoke_ready_callbacks(sys.scheduler, now, invoker);
}

}} // namespace alia

#endif
