#include <alia/platforms/web/host.h>

#include <alia/abi/prelude.h>
#include <alia/abi/ui/input/constants.h>
#include <alia/abi/ui/input/scroll.h>
#include <alia/abi/ui/input/touch_gesture.h>
#include <alia/abi/ui/system/input_processing.h>
#include <alia/abi/ui/system/work.h>

#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>

ALIA_EXTERN_C_BEGIN

alia_key_info
alia_web_key_info_from_keyboard_event(EmscriptenKeyboardEvent const* event);

ALIA_EXTERN_C_END

struct alia_web_host
{
    alia_web_host_config config{};
    bool installed = false;
    bool frame_scheduled = false;
    long animation_frame_handle = 0;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE webgl_context = 0;
    double css_width = 0.0;
    double css_height = 0.0;
    // Primary touch finger emulated as the mouse pointer or touch pan scroll.
    bool primary_touch_active = false;
    int primary_touch_id = -1;
    alia_touch_gesture_kind primary_touch_gesture = ALIA_TOUCH_GESTURE_NONE;
    alia_vec2f last_pointer_pos = {};
};

namespace {

char const*
canvas_selector_or_default(char const* selector)
{
    return (selector && selector[0]) ? selector : "#canvas";
}

float
web_normalize_wheel_axis(double delta, unsigned delta_mode)
{
    switch (delta_mode)
    {
        case ALIA_DOM_DELTA_LINE:
            return static_cast<float>(delta * ALIA_SCROLL_PIXELS_PER_LINE);
        case ALIA_DOM_DELTA_PAGE:
            return static_cast<float>(delta * ALIA_SCROLL_PIXELS_PER_PAGE);
        case ALIA_DOM_DELTA_PIXEL:
        default:
            return static_cast<float>(delta);
    }
}

void
web_host_schedule_frame(alia_web_host* host);

void
web_host_focus_canvas(alia_web_host* host)
{
    ALIA_ASSERT(host);
    char const* const selector
        = canvas_selector_or_default(host->config.canvas_selector);
    EM_ASM(
        {
            var canvas = document.querySelector(UTF8ToString($0));
            if (canvas)
                canvas.focus();
        },
        selector);
}

alia_kmods_t
web_mods_from_emscripten(bool shift, bool ctrl, bool alt, bool meta)
{
    alia_kmods_t mods = 0;
    if (shift)
        mods |= ALIA_KMOD_SHIFT;
    if (ctrl)
        mods |= ALIA_KMOD_CTRL;
    if (alt)
        mods |= ALIA_KMOD_ALT;
    if (meta)
        mods |= ALIA_KMOD_WIN;
    return mods;
}

void
web_host_record_pointer(alia_web_host* host, alia_vec2f pos)
{
    host->last_pointer_pos = pos;
}

void
web_host_enqueue_motion(alia_web_host* host, alia_vec2f pos)
{
    web_host_record_pointer(host, pos);
    alia_ui_enqueue_mouse_motion(host->config.ui, pos);
}

void
web_host_enqueue_press(
    alia_web_host* host,
    alia_vec2f pos,
    alia_button_t button,
    alia_kmods_t mods)
{
    web_host_record_pointer(host, pos);
    alia_ui_enqueue_mouse_motion(host->config.ui, pos);
    alia_ui_enqueue_mouse_press(host->config.ui, pos, button, mods);
}

void
web_host_enqueue_release(
    alia_web_host* host,
    alia_vec2f pos,
    alia_button_t button,
    alia_kmods_t mods)
{
    web_host_record_pointer(host, pos);
    alia_ui_enqueue_mouse_motion(host->config.ui, pos);
    alia_ui_enqueue_mouse_release(host->config.ui, pos, button, mods);
}

void
web_host_sync_surface_internal(
    alia_web_host* host,
    alia_ui_system* ui,
    char const* canvas_selector,
    bool mark_dirty)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(ui);

    char const* const selector = canvas_selector_or_default(canvas_selector);

    double css_w = 0.0;
    double css_h = 0.0;
    emscripten_get_element_css_size(selector, &css_w, &css_h);

    double const dpr = emscripten_get_device_pixel_ratio();

    int const target_w = static_cast<int>(std::lround(css_w * dpr));
    int const target_h = static_cast<int>(std::lround(css_h * dpr));

    emscripten_set_canvas_element_size(selector, target_w, target_h);

    alia_ui_surface_set_size(ui, {target_w, target_h});
    alia_ui_surface_set_dpi(ui, static_cast<float>(dpr * 96.0));

    host->css_width = css_w;
    host->css_height = css_h;

    if (mark_dirty)
        alia_ui_mark_dirty(ui);
}

alia_vec2f
web_pointer_to_framebuffer(
    alia_web_host* host,
    char const* canvas_selector,
    double target_x,
    double target_y)
{
    ALIA_ASSERT(host);

    char const* const selector = canvas_selector_or_default(canvas_selector);

    int backing_w = 0;
    int backing_h = 0;
    emscripten_get_canvas_element_size(selector, &backing_w, &backing_h);

    double css_w = host->css_width;
    double css_h = host->css_height;
    if (css_w <= 0.0 || css_h <= 0.0)
        emscripten_get_element_css_size(selector, &css_w, &css_h);

    if (css_w <= 0.0 || css_h <= 0.0 || backing_w <= 0 || backing_h <= 0)
        return ALIA_BRACED_INIT(alia_vec2f, 0.f, 0.f);

    float const scale_x
        = static_cast<float>(backing_w) / static_cast<float>(css_w);
    float const scale_y
        = static_cast<float>(backing_h) / static_cast<float>(css_h);

    return ALIA_BRACED_INIT(
        alia_vec2f,
        static_cast<float>(target_x) * scale_x,
        static_cast<float>(target_y) * scale_y);
}

bool
host_should_tick(alia_web_host* host)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(host->config.ui);
    if (host->config.continuous)
        return true;
    if (alia_ui_needs_tick(host->config.ui))
        return true;
    alia_nanosecond_count wake_ns = 0;
    return alia_ui_next_wake_ns(host->config.ui, &wake_ns);
}

void
web_host_make_context_current(alia_web_host* host)
{
    ALIA_ASSERT(host);
    if (host->webgl_context)
        emscripten_webgl_make_context_current(host->webgl_context);
}

bool
web_host_animation_frame(double /*time*/, void* user_data)
{
    auto* host = static_cast<alia_web_host*>(user_data);
    ALIA_ASSERT(host);
    host->frame_scheduled = false;
    host->animation_frame_handle = 0;

    if (host_should_tick(host))
    {
        ALIA_ASSERT(host->config.frame.fn);
        web_host_make_context_current(host);
        host->config.frame.fn(host->config.frame.user_data);
    }

    if (host_should_tick(host))
        web_host_schedule_frame(host);

    return true;
}

void
web_host_schedule_frame(alia_web_host* host)
{
    ALIA_ASSERT(host);
    if (host->frame_scheduled)
        return;
    host->frame_scheduled = true;
    host->animation_frame_handle
        = emscripten_request_animation_frame(web_host_animation_frame, host);
}

bool
resize_callback(
    int /*event_type*/, EmscriptenUiEvent const* /*ui_event*/, void* user_data)
{
    auto* host = static_cast<alia_web_host*>(user_data);
    web_host_sync_surface_internal(
        host, host->config.ui, host->config.canvas_selector, true);
    web_host_schedule_frame(host);
    return true;
}

bool
fullscreen_callback(
    int /*event_type*/,
    EmscriptenFullscreenChangeEvent const* /*event*/,
    void* user_data)
{
    auto* host = static_cast<alia_web_host*>(user_data);
    web_host_sync_surface_internal(
        host, host->config.ui, host->config.canvas_selector, true);
    web_host_schedule_frame(host);
    return true;
}

bool
mouse_callback(
    int event_type, EmscriptenMouseEvent const* event, void* user_data)
{
    auto* host = static_cast<alia_web_host*>(user_data);
    ALIA_ASSERT(host->config.ui);
    ALIA_ASSERT(event);

    // Touch is translated to pointer events directly; ignore synthetic mouse
    // events that mobile browsers fire after touch.
    if (host->primary_touch_active)
        return true;

    alia_vec2f const p = web_pointer_to_framebuffer(
        host, host->config.canvas_selector, event->targetX, event->targetY);
    alia_kmods_t const mods = web_mods_from_emscripten(
        event->shiftKey, event->ctrlKey, event->altKey, event->metaKey);

    switch (event_type)
    {
        case EMSCRIPTEN_EVENT_MOUSEMOVE:
            web_host_enqueue_motion(host, p);
            break;
        case EMSCRIPTEN_EVENT_MOUSEDOWN:
            web_host_enqueue_press(host, p, event->button, mods);
            break;
        case EMSCRIPTEN_EVENT_MOUSEUP:
            web_host_enqueue_release(host, p, event->button, mods);
            break;
        case EMSCRIPTEN_EVENT_DBLCLICK:
            web_host_record_pointer(host, p);
            alia_ui_enqueue_mouse_motion(host->config.ui, p);
            alia_ui_enqueue_double_click(
                host->config.ui, p, event->button, mods);
            break;
        default:
            return false;
    }

    web_host_schedule_frame(host);
    return true;
}

bool
window_mouseup_callback(
    int event_type, EmscriptenMouseEvent const* event, void* user_data)
{
    auto* host = static_cast<alia_web_host*>(user_data);
    ALIA_ASSERT(host->config.ui);
    ALIA_ASSERT(event);

    if (event_type != EMSCRIPTEN_EVENT_MOUSEUP)
        return false;

    if (host->primary_touch_active)
        return true;

    alia_kmods_t const mods = web_mods_from_emscripten(
        event->shiftKey, event->ctrlKey, event->altKey, event->metaKey);
    web_host_enqueue_release(
        host, host->last_pointer_pos, event->button, mods);
    web_host_schedule_frame(host);
    return true;
}

alia_vec2f
web_touch_point_to_framebuffer(
    alia_web_host* host, EmscriptenTouchPoint const& touch)
{
    if (touch.onTarget)
    {
        return web_pointer_to_framebuffer(
            host, host->config.canvas_selector, touch.targetX, touch.targetY);
    }
    return host->last_pointer_pos;
}

void
web_host_begin_primary_touch(
    alia_web_host* host, alia_vec2f pos, alia_kmods_t mods)
{
    alia_ui_touch_gesture_resolution const resolution
        = alia_ui_resolve_touch_gesture(host->config.ui, pos);

    host->primary_touch_active = true;
    host->primary_touch_gesture = resolution.kind;
    web_host_record_pointer(host, pos);

    switch (resolution.kind)
    {
        case ALIA_TOUCH_GESTURE_POINTER:
            web_host_enqueue_press(host, pos, ALIA_BUTTON_LEFT, mods);
            break;
        case ALIA_TOUCH_GESTURE_PAN_SCROLL:
            alia_ui_enqueue_mouse_motion(host->config.ui, pos);
            break;
        default:
            break;
    }
}

void
web_host_move_primary_touch(alia_web_host* host, alia_vec2f pos)
{
    switch (host->primary_touch_gesture)
    {
        case ALIA_TOUCH_GESTURE_PAN_SCROLL: {
            alia_vec2f const delta = ALIA_BRACED_INIT(
                alia_vec2f,
                pos.x - host->last_pointer_pos.x,
                pos.y - host->last_pointer_pos.y);
            web_host_record_pointer(host, pos);
            alia_ui_enqueue_mouse_motion(host->config.ui, pos);
            alia_ui_enqueue_scroll(host->config.ui, delta);
            break;
        }
        case ALIA_TOUCH_GESTURE_POINTER:
        default:
            web_host_enqueue_motion(host, pos);
            break;
    }
}

void
web_host_end_primary_touch(
    alia_web_host* host, alia_vec2f pos, alia_kmods_t mods)
{
    if (host->primary_touch_gesture == ALIA_TOUCH_GESTURE_POINTER)
        web_host_enqueue_release(host, pos, ALIA_BUTTON_LEFT, mods);

    host->primary_touch_active = false;
    host->primary_touch_id = -1;
    host->primary_touch_gesture = ALIA_TOUCH_GESTURE_NONE;
}

bool
touch_callback(
    int event_type, EmscriptenTouchEvent const* event, void* user_data)
{
    auto* host = static_cast<alia_web_host*>(user_data);
    ALIA_ASSERT(host->config.ui);
    ALIA_ASSERT(event);

    alia_kmods_t const mods = web_mods_from_emscripten(
        event->shiftKey, event->ctrlKey, event->altKey, event->metaKey);

    for (int i = 0; i < event->numTouches; ++i)
    {
        EmscriptenTouchPoint const& touch = event->touches[i];
        if (!touch.isChanged)
            continue;

        switch (event_type)
        {
            case EMSCRIPTEN_EVENT_TOUCHSTART:
                if (!host->primary_touch_active)
                {
                    host->primary_touch_id = touch.identifier;
                    alia_vec2f const p
                        = web_touch_point_to_framebuffer(host, touch);
                    web_host_focus_canvas(host);
                    web_host_begin_primary_touch(host, p, mods);
                }
                break;

            case EMSCRIPTEN_EVENT_TOUCHMOVE:
                if (host->primary_touch_active
                    && touch.identifier == host->primary_touch_id)
                {
                    alia_vec2f const p
                        = web_touch_point_to_framebuffer(host, touch);
                    web_host_move_primary_touch(host, p);
                }
                break;

            case EMSCRIPTEN_EVENT_TOUCHEND:
            case EMSCRIPTEN_EVENT_TOUCHCANCEL:
                if (host->primary_touch_active
                    && touch.identifier == host->primary_touch_id)
                {
                    alia_vec2f const p
                        = web_touch_point_to_framebuffer(host, touch);
                    web_host_end_primary_touch(host, p, mods);
                }
                break;

            default:
                return false;
        }
    }

    web_host_schedule_frame(host);
    return true;
}

bool
wheel_callback(
    int /*event_type*/, EmscriptenWheelEvent const* event, void* user_data)
{
    auto* host = static_cast<alia_web_host*>(user_data);
    ALIA_ASSERT(host->config.ui);
    ALIA_ASSERT(event);

    unsigned const delta_mode = static_cast<unsigned>(event->deltaMode);
    alia_vec2f const delta = ALIA_BRACED_INIT(
        alia_vec2f,
        web_normalize_wheel_axis(event->deltaX, delta_mode),
        web_normalize_wheel_axis(event->deltaY, delta_mode));
    alia_ui_enqueue_scroll(host->config.ui, delta);
    web_host_schedule_frame(host);
    return true;
}

bool
key_callback(
    int event_type, EmscriptenKeyboardEvent const* event, void* user_data)
{
    auto* host = static_cast<alia_web_host*>(user_data);
    ALIA_ASSERT(host->config.ui);
    ALIA_ASSERT(event);

    alia_key_info const key = alia_web_key_info_from_keyboard_event(event);
    if (key.fields_present == 0)
        return false;

    switch (event_type)
    {
        case EMSCRIPTEN_EVENT_KEYDOWN:
            if (event->repeat)
                return true;
            alia_ui_enqueue_key_press(host->config.ui, key);
            break;
        case EMSCRIPTEN_EVENT_KEYUP:
            alia_ui_enqueue_key_release(host->config.ui, key);
            break;
        default:
            return false;
    }

    web_host_schedule_frame(host);
    return true;
}

} // namespace

extern "C" {

alia_web_host*
alia_web_host_create(void)
{
    auto* host = new (std::nothrow) alia_web_host{};
    ALIA_ASSERT(host);
    return host;
}

void
alia_web_host_destroy(alia_web_host* host)
{
    ALIA_ASSERT(host);

    if (host->frame_scheduled && host->animation_frame_handle)
        emscripten_cancel_animation_frame(host->animation_frame_handle);

    if (host->webgl_context)
    {
        emscripten_webgl_destroy_context(host->webgl_context);
        host->webgl_context = 0;
    }

    delete host;
}

bool
alia_web_host_init_webgl(alia_web_host* host, char const* canvas_selector)
{
    ALIA_ASSERT(host);

    char const* const selector = canvas_selector_or_default(canvas_selector);

    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.alpha = EM_FALSE;
    attrs.depth = EM_FALSE;
    attrs.stencil = EM_FALSE;
    attrs.antialias = EM_FALSE;
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE const ctx
        = emscripten_webgl_create_context(selector, &attrs);
    if (ctx <= 0)
    {
        std::fprintf(stderr, "alia_web_host: WebGL context creation failed\n");
        return false;
    }

    if (emscripten_webgl_make_context_current(ctx)
        != EMSCRIPTEN_RESULT_SUCCESS)
    {
        std::fprintf(
            stderr, "alia_web_host: failed to make WebGL context current\n");
        emscripten_webgl_destroy_context(ctx);
        return false;
    }

    if (host->webgl_context)
        emscripten_webgl_destroy_context(host->webgl_context);

    host->webgl_context = ctx;
    return true;
}

void
alia_web_host_sync_surface(
    alia_web_host* host, alia_ui_system* ui, char const* canvas_selector)
{
    web_host_sync_surface_internal(host, ui, canvas_selector, true);
}

void
alia_web_host_install(alia_web_host* host, alia_web_host_config const* config)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(config);
    ALIA_ASSERT(config->ui);

    host->config = *config;
    if (!host->config.canvas_selector)
        host->config.canvas_selector = "#canvas";

    char const* const selector
        = canvas_selector_or_default(host->config.canvas_selector);
    char const* const target = selector;

    if (host->installed)
        return;

    emscripten_set_resize_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, host, false, resize_callback);
    emscripten_set_fullscreenchange_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, host, false, fullscreen_callback);

    emscripten_set_mousemove_callback(target, host, false, mouse_callback);
    emscripten_set_mousedown_callback(target, host, false, mouse_callback);
    emscripten_set_mouseup_callback(target, host, false, mouse_callback);
    emscripten_set_dblclick_callback(target, host, false, mouse_callback);
    emscripten_set_mouseup_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, host, true, window_mouseup_callback);
    emscripten_set_wheel_callback(target, host, false, wheel_callback);
    emscripten_set_touchstart_callback(target, host, false, touch_callback);
    emscripten_set_touchmove_callback(target, host, false, touch_callback);
    emscripten_set_touchend_callback(target, host, false, touch_callback);
    emscripten_set_touchcancel_callback(target, host, false, touch_callback);
    emscripten_set_touchend_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, host, true, touch_callback);
    emscripten_set_touchcancel_callback(
        EMSCRIPTEN_EVENT_TARGET_WINDOW, host, true, touch_callback);
    emscripten_set_keydown_callback(target, host, false, key_callback);
    emscripten_set_keyup_callback(target, host, false, key_callback);

    host->installed = true;
}

void
alia_web_host_request_frame(alia_web_host* host)
{
    ALIA_ASSERT(host);
    web_host_schedule_frame(host);
}

void
alia_web_host_run(alia_web_host* host, alia_web_host_config const* config)
{
    ALIA_ASSERT(host);
    ALIA_ASSERT(config);
    ALIA_ASSERT(config->ui);
    ALIA_ASSERT(config->frame.fn);

    alia_web_host_install(host, config);
    web_host_sync_surface_internal(
        host, config->ui, config->canvas_selector, false);
    web_host_schedule_frame(host);
}

} // extern "C"
