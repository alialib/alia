#pragma once

#include <alia/abi/ui/trace.h>
#include <alia/ui/system/object.h>

namespace alia {

// RAII helper that timestamps and emits trace info for passes - The caller
// should fill in pass-specific fields while the scope is live. The trace info
// emitted on destruction.
struct trace_pass_builder
{
    ui_system* ui = nullptr;
    alia_trace_pass pass{};

    trace_pass_builder(ui_system& sys, alia_trace_pass_kind kind)
    {
        if (!sys.trace_sink || !sys.trace_sink->pass)
            return;
        ui = &sys;
        pass.kind = kind;
        pass.start = alia_trace_now();
    }

    trace_pass_builder(trace_pass_builder const&) = delete;
    trace_pass_builder&
    operator=(trace_pass_builder const&) = delete;

    bool
    active() const
    {
        return ui != nullptr;
    }

    ~trace_pass_builder()
    {
        if (!ui)
            return;
        pass.end = alia_trace_now();
        ui->trace_sink->pass(ui->trace_sink->user, &pass);
    }
};

} // namespace alia
