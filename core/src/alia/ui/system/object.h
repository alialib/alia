#pragma once

// #include <alia/core/system/internals.hpp>
// #include <alia/core/timing/ticks.hpp>
#include <alia/abi/base/geometry.h>
#include <alia/abi/kernel/routing.h>
#include <alia/abi/ui/input/state.h>
#include <alia/abi/ui/msdf.h>
#include <alia/abi/ui/text.h>
#include <alia/context.h>
#include <alia/impl/events.hpp>
#include <alia/kernel/object.h>
#include <alia/ui/layout/system.h>
// #include <alia/system/os_interface.hpp>
// #include <alia/system/window_interface.hpp>
#include <alia/abi/ui/icons.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/host_window.h>
#include <alia/abi/ui/system/renderer.h>
#include <alia/abi/ui/system/work.h>
#include <alia/abi/ui/trace.h>
#include <alia/ui/drawing/system.h>
#include <alia/ui/styling.h>

#include <cstdint>
#include <deque>
#include <vector>

extern "C" {

struct alia_ui_system
{
    // persistent kernel state for this UI
    alia_kernel kernel;

    alia_draw_system draw;

    alia_ui_controller controller{};

    alia_host_window_ops host_window{};

    alia_renderer_ops renderer{};

    // alia__shared_ptr<alia::surface> surface;
    alia_vec2i surface_size;
    float dpi;
    // UI magnification (independent of DPI)
    float magnification = 1;

    // std::shared_ptr<os_interface> os;

    // std::shared_ptr<window_interface> window;

    alia_input_state input;

    alia_layout_system layout;

    alia_palette palette;

    alia::style_catalog styles;

    // core icons - These sepcify fonts and codepoints for icons that are
    // referenced by core components.
    alia_ui_icon icons[ALIA_UI_ICON_COUNT]{};

    // optional MSDF text engine used by core widgets that render glyphs
    alia_msdf_text_engine* msdf_text_engine = nullptr;

    // typeface registry
    std::vector<alia_resolved_typeface> typefaces;

    // pending dispatch events (input and other queueable work)
    std::deque<alia_event> event_queue;

    alia_ui_refresh_policy refresh_policy{};

    // optional trace sink - This is NULL when tracing is off.
    alia_ui_trace_sink const* trace_sink = nullptr;
};

} // extern "C"
