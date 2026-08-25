#include <alia/abi/ui/system/tracer.h>

#include <cstring>

namespace {

void
tracer_frame_begin(void* user)
{
    auto* tracer = static_cast<alia_ui_tracer*>(user);
    ALIA_ASSERT(tracer);
    ALIA_ASSERT(!tracer->in_frame);

    alia_trace_frame* frame = &tracer->frames[tracer->write_index];
    std::memset(frame, 0, sizeof(*frame));
    frame->index = tracer->next_frame_index++;
    frame->wall_start = alia_trace_now();

    tracer->current = frame;
    tracer->in_frame = true;
}

void
tracer_frame_end(void* user)
{
    auto* tracer = static_cast<alia_ui_tracer*>(user);
    ALIA_ASSERT(tracer);
    if (!tracer->in_frame || !tracer->current)
        return;

    tracer->current->wall_end = alia_trace_now();
    tracer->current = nullptr;
    tracer->in_frame = false;

    tracer->write_index
        = (tracer->write_index + 1u) % ALIA_TRACE_RING_SIZE;
    if (tracer->stored_count < ALIA_TRACE_RING_SIZE)
        ++tracer->stored_count;
}

void
tracer_pass(void* user, alia_trace_pass const* pass)
{
    auto* tracer = static_cast<alia_ui_tracer*>(user);
    ALIA_ASSERT(tracer);
    ALIA_ASSERT(pass);
    if (!tracer->in_frame || !tracer->current)
        return;

    alia_trace_frame* frame = tracer->current;
    if (frame->pass_count >= ALIA_TRACE_MAX_PASSES)
    {
        ++frame->dropped_passes;
        return;
    }
    frame->passes[frame->pass_count++] = *pass;
}

} // namespace

extern "C" {

void
alia_ui_tracer_init(alia_ui_tracer* tracer)
{
    ALIA_ASSERT(tracer);
    std::memset(tracer, 0, sizeof(*tracer));
    tracer->sink.user = tracer;
    tracer->sink.frame_begin = tracer_frame_begin;
    tracer->sink.frame_end = tracer_frame_end;
    tracer->sink.pass = tracer_pass;
}

void
alia_ui_tracer_attach(alia_ui_tracer* tracer, alia_ui_system* ui)
{
    ALIA_ASSERT(tracer);
    ALIA_ASSERT(ui);
    alia_ui_set_trace_sink(ui, &tracer->sink);
}

void
alia_ui_tracer_detach(alia_ui_tracer* tracer, alia_ui_system* ui)
{
    ALIA_ASSERT(tracer);
    ALIA_ASSERT(ui);
    if (alia_ui_get_trace_sink(ui) == &tracer->sink)
        alia_ui_set_trace_sink(ui, nullptr);
}

alia_trace_frame const*
alia_ui_tracer_latest(alia_ui_tracer const* tracer)
{
    return alia_ui_tracer_frame_ago(tracer, 0);
}

alia_trace_frame const*
alia_ui_tracer_frame_ago(alia_ui_tracer const* tracer, uint32_t steps)
{
    ALIA_ASSERT(tracer);
    if (tracer->stored_count == 0 || steps >= tracer->stored_count)
        return nullptr;

    uint32_t const latest
        = (tracer->write_index + ALIA_TRACE_RING_SIZE - 1u)
        % ALIA_TRACE_RING_SIZE;
    uint32_t const index
        = (latest + ALIA_TRACE_RING_SIZE - steps) % ALIA_TRACE_RING_SIZE;
    return &tracer->frames[index];
}

char const*
alia_trace_pass_kind_name(alia_trace_pass_kind kind)
{
    switch (kind)
    {
        case ALIA_TRACE_PASS_REFRESH:
            return "REFRESH";
        case ALIA_TRACE_PASS_EVENT:
            return "EVENT";
        case ALIA_TRACE_PASS_LAYOUT:
            return "LAYOUT";
        case ALIA_TRACE_PASS_DRAW_RECORD:
            return "DRAW_RECORD";
        case ALIA_TRACE_PASS_DRAW_EXECUTE:
            return "DRAW_EXECUTE";
        default:
            return "?";
    }
}

} // extern "C"
