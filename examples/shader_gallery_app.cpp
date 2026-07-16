#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>

#include <alia/shell/app.h>
#if defined(ALIA_SHELL_BACKEND_D3D11)
#include <alia/renderers/d3d11/renderer.h>
#else
#include <alia/renderers/gl/renderer.h>
#endif

#include <alia/abi/base/arena.h>
#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/effects.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/input/pointer.h>
#include <alia/abi/ui/input/regions.h>
#include <alia/abi/ui/layout/system.h>
#include <alia/abi/ui/library.h>
#include <alia/abi/ui/palette.h>
#include <alia/abi/ui/style.h>
#include <alia/abi/ui/system/api.h>
#include <alia/abi/ui/system/input_processing.h>
#include <alia/base/color.hpp>
#include <alia/context.h>
#include <alia/impl/base/arena.hpp>
#include <alia/impl/events.hpp>
#include <alia/impl/ui/layout.hpp>
#include <alia/kernel/flow/dispatch.h>
#include <alia/kernel/macros.hpp>
#include <alia/prelude.hpp>
#include <alia/ui/drawing.h>
#include <alia/ui/layout/api.hpp>
#include <alia/ui/library.hpp>
#include <alia/ui/system/internal_api.h>
#include <alia/ui/system/object.h>

using namespace alia;
using namespace alia::operators;

alia_msdf_text_engine* the_msdf_text_engine = nullptr;

#include "prototyping/msdf.h"
#include "prototyping/panel.h"

// Param blob after the backend region header. Layout matches float4 packs in
// both HLSL cbuffer b0 and GLSL std140 block `Effect`.
struct notargs_effect_params
{
    float zoom;
    float t;
    float curl;
    float thickness;

    float cosx_factor;
    float cosy_factor;
    float siny_factor;
    float sinz_factor;

    float normalize_rays;
    float step_scaling;
    float iterations;
    float invert;

    float color_r;
    float color_g;
    float color_b;
    float color_factor;

    float sine_factor;
    float depth_scale;
    float pad0;
    float pad1;
};

static_assert(sizeof(notargs_effect_params) == 80);

#if defined(ALIA_SHELL_BACKEND_D3D11)
char const* const notargs_ps_hlsl = R"(
cbuffer AliaEffectFrame : register(b0)
{
    float4 alia_effect_region;
    float4 alia_effect_surface;
};

cbuffer Effect : register(b1)
{
    float4 p0;
    float4 p1;
    float4 p2;
    float4 p3;
    float4 p4;
};

static const float tau = 6.28318530718;

float sdf(float3 p)
{
    float t = p0.y;
    float curl = p0.z;
    float thickness = p0.w;
    float cosx_factor = p1.x;
    float cosy_factor = p1.y;
    float siny_factor = p1.z;
    float sinz_factor = p1.w;

    p.z -= t;
    float a = fmod(p.z * 0.1, tau) * curl;
    float cs = cos(a);
    float sn = sin(a);
    float2 xy = float2(cs * p.x - sn * p.y, sn * p.x + cs * p.y);
    p.xy = xy;
    return thickness
         - length(
               float2(cos(p.x) * cosx_factor, cos(p.y) * cosy_factor)
               + float2(sin(p.y) * siny_factor, sin(p.z) * sinz_factor));
}

float4 ps_main(float4 frag_coord : SV_POSITION) : SV_TARGET
{
    float2 corner = alia_effect_region.xy;
    float2 size = alia_effect_region.zw;
    float zoom = p0.x;
    bool normalize_rays = p2.x > 0.5;
    float step_scaling = p2.y;
    int iterations = int(p2.z + 0.5);
    bool invert = p2.w > 0.5;
    float3 color = p3.xyz;
    float color_factor = p3.w;
    float sine_factor = p4.x;
    float depth_scale = p4.y;

    float2 half_size = size / 2.0;
    float2 uv = (frag_coord.xy - corner - half_size) / (zoom * half_size.y);

    float3 direction;
    if (normalize_rays)
        direction = normalize(float3(uv, 1.0)) * step_scaling;
    else
        direction = float3(uv, step_scaling);

    float distance_traveled = 0.0;
    for (int i = 0; i < iterations; ++i)
        distance_traveled += sdf(direction * distance_traveled);

    float3 p = direction * distance_traveled;
    float depth_factor = invert ? (length(p) * depth_scale / 120.0)
                                : (1.0 / (length(p) * depth_scale));
    return float4(
        (sin(p) * sine_factor + color * color_factor) * depth_factor, 1.0);
}
)";
#else
char const* const notargs_frag_src = R"(
uniform vec4 alia_effect_region;
uniform vec4 alia_effect_surface;

layout(std140) uniform Effect {
    vec4 p0;
    vec4 p1;
    vec4 p2;
    vec4 p3;
    vec4 p4;
};

out vec4 frag_color;

const float tau = radians(360.0);

float sdf(vec3 p)
{
    float t = p0.y;
    float curl = p0.z;
    float thickness = p0.w;
    float cosx_factor = p1.x;
    float cosy_factor = p1.y;
    float siny_factor = p1.z;
    float sinz_factor = p1.w;

    p.z -= t;
    float a = mod(p.z * 0.1, tau) * curl;
    p.xy *= mat2(cos(a), sin(a), -sin(a), cos(a));
    return thickness
        - length(
               vec2(cos(p.x) * cosx_factor, cos(p.y) * cosy_factor)
               + vec2(sin(p.y) * siny_factor, sin(p.z) * sinz_factor));
}

void main()
{
    vec2 corner = alia_effect_region.xy;
    vec2 size = alia_effect_region.zw;
    float zoom = p0.x;
    bool normalize_rays = p2.x > 0.5;
    float step_scaling = p2.y;
    int iterations = int(p2.z + 0.5);
    bool invert = p2.w > 0.5;
    vec3 color = p3.xyz;
    float color_factor = p3.w;
    float sine_factor = p4.x;
    float depth_scale = p4.y;

    // Map GL bottom-left fragment coords into Alia surface space.
    vec2 frag = vec2(gl_FragCoord.x, alia_effect_surface.y - gl_FragCoord.y);

    vec2 half_size = size / 2.0;
    vec2 uv = (frag - corner - half_size) / (zoom * half_size.y);

    vec3 direction;
    if (normalize_rays)
        direction = normalize(vec3(uv, 1.0)) * step_scaling;
    else
        direction = vec3(uv, step_scaling);

    float distance_traveled = 0.0;
    for (int i = 0; i < iterations; ++i)
        distance_traveled += sdf(direction * distance_traveled);

    vec3 p = vec3(direction * distance_traveled);
    float depth_factor = invert ? (length(p) * depth_scale / 120.0)
                                : (1.0 / (length(p) * depth_scale));
    frag_color = vec4(
        (sin(p) * sine_factor + color * color_factor) * depth_factor, 1.0);
}
)";
#endif

struct notargs_controls
{
    double zoom = 1.0;
    bool animate = true;
    double speed = 2.5;
    double curl = 1.0;
    double thickness = 0.1;
    bool include_cosx = true;
    bool include_cosy = true;
    bool include_siny = true;
    bool include_sinz = true;
    bool normalize_rays = false;
    double step_scaling = 0.5;
    double iterations = 32.0;
    double color_factor = 1.0;
    double sine_factor = 1.0;
    double depth_scale = 1.0;
};

notargs_controls the_controls;
double the_notargs_t = 0.0;
int the_seed_index = 0;
bool the_light_theme_flag = true;
bool the_theme_dirty_flag = true;

alia_srgb8 const the_seed_primaries[]
    = {hex_color("#154DCF"), hex_color("#6f42c1"), hex_color("#a52e45")};

alia_ui_system* the_system = nullptr;
alia_style the_style = {.spacing = 10.0f};
std::chrono::high_resolution_clock::time_point the_last_anim_time
    = std::chrono::high_resolution_clock::now();

alia_draw_material_id the_notargs_effect_id = 0;

void
shader_color_for_seed(int seed_index, float* out_rgb)
{
    switch (seed_index)
    {
        case 1:
            out_rgb[0] = 5.f;
            out_rgb[1] = 2.f;
            out_rgb[2] = 7.f;
            return;
        case 2:
            out_rgb[0] = 9.f;
            out_rgb[1] = 4.f;
            out_rgb[2] = 2.f;
            return;
        case 3:
            out_rgb[0] = 3.f;
            out_rgb[1] = 7.f;
            out_rgb[2] = 9.f;
            return;
        case 0:
        default:
            out_rgb[0] = 2.f;
            out_rgb[1] = 5.f;
            out_rgb[2] = 11.f;
            return;
    }
}

notargs_effect_params
make_notargs_params()
{
    notargs_effect_params p{};
    p.zoom = float(std::exp(the_controls.zoom));
    p.t = float(the_notargs_t);
    p.curl = float(the_controls.curl);
    p.thickness = float(the_controls.thickness);
    p.cosx_factor = the_controls.include_cosx ? 1.0f : 0.0f;
    p.cosy_factor = the_controls.include_cosy ? 1.0f : 0.0f;
    p.siny_factor = the_controls.include_siny ? 1.0f : 0.0f;
    p.sinz_factor = the_controls.include_sinz ? 1.0f : 0.0f;
    p.normalize_rays = the_controls.normalize_rays ? 1.0f : 0.0f;
    p.step_scaling = float(the_controls.step_scaling);
    p.iterations = float(
        std::lround(std::clamp(the_controls.iterations, 0.0, 120.0)));
    p.invert = the_light_theme_flag ? 0.0f : 1.0f;
    float rgb[3];
    shader_color_for_seed(the_seed_index, rgb);
    p.color_r = rgb[0];
    p.color_g = rgb[1];
    p.color_b = rgb[2];
    p.color_factor = float(the_controls.color_factor);
    p.sine_factor = float(the_controls.sine_factor);
    p.depth_scale = float(the_controls.depth_scale);
    return p;
}

struct pass_aborted
{
};

void
abort_pass(context& ctx)
{
    ctx.events->aborted = true;
    throw pass_aborted();
}

template<class Content>
void
with_spacing(context& ctx, float spacing, Content&& content)
{
    float old_spacing = ctx.style->spacing;
    ctx.style->spacing = spacing * ctx.geometry->scale;
    content();
    ctx.style->spacing = old_spacing;
}

void
do_heading(context& ctx, char const* text)
{
    do_text(
        ctx,
        2,
        alia_srgba8_from_srgb8(ctx.palette->foundation.text.stronger_2),
        18.f,
        text,
        NO_FLAGS,
        1);
}

void
do_radio_with_text(context& ctx, alia_bool_signal* value, char const* text)
{
    alia_box row_box;
    row(ctx, ALIGN_LEFT, &row_box, [&]() {
        alia_element_id id
            = alia_do_radio(&ctx, value, ALIA_CENTER_Y, nullptr);
        do_text(
            ctx,
            2,
            alia_srgba8_from_srgb8(
                value->flags & ALIA_SIGNAL_WRITABLE
                    ? ctx.palette->foundation.text.base
                    : ctx.palette->foundation.text.weaker_2),
            14.f,
            text,
            CENTER_Y);
        alia_element_box_region(
            &ctx, id, &row_box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
    });
}

void
do_switch_with_text(context& ctx, alia_bool_signal* value, char const* text)
{
    alia_box row_box;
    row(ctx, ALIGN_LEFT, &row_box, [&]() {
        alia_element_id id
            = alia_do_switch(&ctx, value, ALIA_CENTER_Y, nullptr);
        do_text(
            ctx,
            2,
            alia_srgba8_from_srgb8(
                value->flags & ALIA_SIGNAL_WRITABLE
                    ? ctx.palette->foundation.text.base
                    : ctx.palette->foundation.text.weaker_2),
            14.f,
            text,
            CENTER_Y);
        alia_element_box_region(
            &ctx, id, &row_box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
    });
}

void
do_checkbox_with_text(context& ctx, alia_bool_signal* value, char const* text)
{
    alia_box row_box;
    row(ctx, ALIGN_LEFT, &row_box, [&]() {
        alia_element_id id
            = alia_do_checkbox(&ctx, value, ALIA_CENTER_Y, nullptr);
        do_text(
            ctx,
            2,
            alia_srgba8_from_srgb8(
                value->flags & ALIA_SIGNAL_WRITABLE
                    ? ctx.palette->foundation.text.base
                    : ctx.palette->foundation.text.weaker_2),
            14.f,
            text,
            CENTER_Y);
        alia_element_box_region(
            &ctx, id, &row_box, ALIA_CURSOR_DEFAULT, ALIA_HIT_TEST_MOUSE);
    });
}

void
control_slider_d(
    context& ctx,
    char const* label,
    double* value,
    double min_v,
    double max_v,
    double step)
{
    row(ctx, [&]() {
        do_text(
            ctx,
            2,
            alia_srgba8_from_srgb8(ctx.palette->foundation.text.base),
            14.f,
            label,
            CENTER_Y);
        flow(ctx, GROW, [&]() {
            alia_do_slider_d(
                &ctx, value, min_v, max_v, step, 0, false, nullptr);
        });
    });
}

void
control_switch_b(context& ctx, char const* label, bool* value)
{
    row(ctx, [&]() {
        alia_bool_signal sig{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = *value,
        };
        do_switch_with_text(ctx, &sig, label);
        if (sig.flags & ALIA_SIGNAL_WRITTEN)
        {
            *value = sig.value;
            abort_pass(ctx);
        }
    });
}

void
control_checkbox_b(context& ctx, char const* label, bool* value)
{
    row(ctx, [&]() {
        alia_bool_signal sig{
            .flags = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
            .value = *value,
        };
        do_checkbox_with_text(ctx, &sig, label);
        if (sig.flags & ALIA_SIGNAL_WRITTEN)
        {
            *value = sig.value;
            abort_pass(ctx);
        }
    });
}

void
do_theme_controls(context& ctx)
{
    concrete_panel(
        ctx, 0, ctx.palette->foundation.background.stronger_1, FILL, [&]() {
            edge_offsets(
                ctx,
                {.left = 12, .right = 12, .top = 12, .bottom = 12},
                [&]() {
                    row(ctx, [&]() {
                        with_spacing(ctx, 6, [&] {
                            alia_bool_signal sig{
                                .flags
                                = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
                                .value = the_light_theme_flag,
                            };
                            do_switch_with_text(ctx, &sig, "Light");
                            if (sig.flags & ALIA_SIGNAL_WRITTEN)
                            {
                                the_light_theme_flag = sig.value;
                                the_theme_dirty_flag = true;
                                abort_pass(ctx);
                            }
                        });
                        do_text(
                            ctx,
                            2,
                            alia_srgba8_from_srgb8(
                                ctx.palette->foundation.text.base),
                            14.f,
                            " ",
                            CENTER_Y);
                        spacer(ctx, {0, 0}, GROW);
                        with_spacing(ctx, 0, [&] {
                            for (int i = 0; i < 3; ++i)
                            {
                                alia_bool_signal radio{
                                    .flags = ALIA_SIGNAL_READABLE
                                           | ALIA_SIGNAL_WRITABLE,
                                    .value = (the_seed_index == i),
                                };
                                char const* labels[]
                                    = {"Blue", "Violet", "Red"};
                                do_radio_with_text(ctx, &radio, labels[i]);
                                if (radio.flags & ALIA_SIGNAL_WRITTEN)
                                {
                                    the_seed_index = i;
                                    the_theme_dirty_flag = true;
                                    abort_pass(ctx);
                                }
                                spacer(ctx, {15, 0}, NO_FLAGS);
                            }
                        });
                    });
                });
        });
}

void
do_notargs_controls(context& ctx)
{
    do_heading(ctx, "VIEW");
    control_slider_d(ctx, "Zoom", &the_controls.zoom, -3.0, 5.0, 0.01);
    control_switch_b(ctx, "Animate", &the_controls.animate);
    control_slider_d(ctx, "Speed", &the_controls.speed, -2.0, 7.0, 0.1);

    spacer(ctx, alia_vec2f_make(1.f, 28.f));

    do_heading(ctx, "RAY MARCHING");
    control_switch_b(ctx, "Normalize Rays", &the_controls.normalize_rays);
    control_slider_d(
        ctx, "Step Scaling", &the_controls.step_scaling, 0.0, 1.0, 0.001);
    control_slider_d(
        ctx, "Iterations", &the_controls.iterations, 0.0, 120.0, 1.0);

    spacer(ctx, alia_vec2f_make(1.f, 28.f));

    do_heading(ctx, "SHAPE");
    control_slider_d(ctx, "Curl", &the_controls.curl, 0.0, 1.0, 0.001);
    control_slider_d(
        ctx, "Thickness", &the_controls.thickness, 0.0, 2.0, 0.001);
    control_checkbox_b(ctx, "Include CosX", &the_controls.include_cosx);
    control_checkbox_b(ctx, "Include CosY", &the_controls.include_cosy);
    control_checkbox_b(ctx, "Include SinY", &the_controls.include_siny);
    control_checkbox_b(ctx, "Include SinZ", &the_controls.include_sinz);

    spacer(ctx, alia_vec2f_make(1.f, 28.f));

    do_heading(ctx, "COLORING");
    control_slider_d(
        ctx, "Color Factor", &the_controls.color_factor, 0.0, 2.0, 0.001);
    control_slider_d(
        ctx, "Sine Factor", &the_controls.sine_factor, 0.0, 4.0, 0.001);
    control_slider_d(
        ctx, "Depth Scale", &the_controls.depth_scale, 0.0, 4.0, 0.001);
}

void
shader_gallery_root(context& ctx)
{
    try
    {
        if (the_theme_dirty_flag)
        {
            alia_theme_accent accent;
            alia_theme_accent_from_color(
                &accent, the_seed_primaries[the_seed_index]);

            alia_theme_context theme_ctx
                = alia_theme_context_default(!the_light_theme_flag);

            alia_palette_params palette_params = alia_palette_params_default();
            palette_params.foundation_step_l = 0.06f;

            alia_palette_from_accent(
                &ctx.system->palette,
                &accent,
                &theme_ctx,
                nullptr,
                &palette_params,
                ALIA_LITERAL_HARMONIZE_TO_PRIMARY);
            the_theme_dirty_flag = false;
        }

        notargs_effect_params const effect_params = make_notargs_params();

        with_spacing(ctx, 0, [&] {
            row(ctx, [&]() {
                concrete_panel(
                    ctx,
                    0,
                    ctx.palette->foundation.background.base,
                    FILL,
                    [&]() {
                        column(ctx, GROW, [&]() {
                            alia_ui_scroll_view_begin(
                                &ctx, ALIA_GROW, 0x2, 0, nullptr);
                            edge_offsets(
                                ctx,
                                {.left = 20,
                                 .right = 20,
                                 .top = 20,
                                 .bottom = 20},
                                [&]() {
                                    with_spacing(ctx, 6, [&] {
                                        column(ctx, [&]() {
                                            do_notargs_controls(ctx);
                                        });
                                    });
                                });
                            alia_ui_scroll_view_end(&ctx);
                            do_theme_controls(ctx);
                        });
                    });
                concrete_panel(
                    ctx,
                    0,
                    ctx.palette->foundation.background.base,
                    GROW,
                    [&]() {
                        alia_do_effect(
                            &ctx,
                            0,
                            the_notargs_effect_id,
                            &effect_params,
                            sizeof(effect_params),
                            ALIA_GROW | ALIA_FILL,
                            nullptr);
                    });
            });
        });
    }
    catch (pass_aborted&)
    {
    }
}

static void
shader_gallery_root_controller(void* user_data, alia_context* ctx)
{
    (void) user_data;
    shader_gallery_root(*ctx);
}

void
update()
{
    auto const now = std::chrono::high_resolution_clock::now();
    double const delta_ms
        = std::chrono::duration<double, std::milli>(now - the_last_anim_time)
              .count();
    the_last_anim_time = now;

    alia_ui_system_update(the_system);

    if (the_controls.animate)
    {
        the_notargs_t
            += (delta_ms / 1000.0)
             * std::exp(static_cast<double>(the_controls.speed)) * 0.1;
    }

    alia_app_shell_draw(the_system);
}

static void
shader_gallery_frame(void* /*user_data*/)
{
    update();
}

int
main()
{
    static alia_host_window_options const window_options = {
        .resizable = true,
        .vsync = true,
    };

    // Web host returns after scheduling RAF; keep storage for the page
    // lifetime.
    static alia_app app;
    alia_app_config const config = {
        .inner = {shader_gallery_root_controller, nullptr},
        .shell = {
            .draw_foundation_underlay = false,
            .surface_padding = {},
        },
        .frame = {shader_gallery_frame, nullptr},
        .continuous = true,
        .title = "Alia Shader Gallery",
        .window_state = alia_window_state_make(1200, 800),
        .window_options = &window_options,
        .canvas_selector = "#canvas",
    };
    if (alia_app_init(&config, &app) != 0)
        return 1;

    the_system = alia_app_ui(&app);
    the_theme_dirty_flag = true;

    the_notargs_effect_id = 0;
#if defined(ALIA_SHELL_BACKEND_D3D11)
    alia_d3d11_effect_desc const d3d_effect = {
        .pixel_shader_hlsl = notargs_ps_hlsl,
        .entry_point = "ps_main",
        .params_size = sizeof(notargs_effect_params),
    };
    if (alia_d3d11_effect_register(
            alia_app_d3d11_renderer(&app), &d3d_effect, &the_notargs_effect_id)
        != 0)
    {
        std::cerr << "notargs: D3D11 effect registration failed\n";
        alia_app_destroy(&app);
        return 1;
    }
#else
    alia_gl_effect_desc const gl_effect = {
        .fragment_shader_source = notargs_frag_src,
        .params_size = sizeof(notargs_effect_params),
    };
    if (alia_gl_effect_register(
            alia_app_gl_renderer(&app), &gl_effect, &the_notargs_effect_id)
        != 0)
    {
        std::cerr << "notargs: GL effect registration failed\n";
        alia_app_destroy(&app);
        return 1;
    }
#endif

    alia_app_setup_stock_text(&app);
    the_msdf_text_engine = alia_app_text_engine(&app);

    alia_app_run_loop(&config, &app);
#ifndef __EMSCRIPTEN__
    alia_app_destroy(&app);
#endif
    return 0;
}
