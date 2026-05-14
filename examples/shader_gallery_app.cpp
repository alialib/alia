#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

#include "alia_fonts.h"

#include <alia/renderers/gl/renderer.hpp>

#include <alia/abi/base/arena.h>
#include <alia/abi/base/color.h>
#include <alia/abi/base/geometry.h>
#include <alia/abi/ui/drawing.h>
#include <alia/abi/ui/events.h>
#include <alia/abi/ui/input/elements.h>
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
#include <alia/platforms/glfw/input_glue.h>
#include <alia/prelude.hpp>
#include <alia/ui/drawing.h>
#include <alia/ui/layout/components.hpp>
#include <alia/ui/layout/flags.hpp>
#include <alia/ui/library.hpp>
#include <alia/ui/system/internal_api.h>
#include <alia/ui/system/object.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <GLFW/glfw3.h>

#ifndef __EMSCRIPTEN__
#include <glad/glad.h>
#endif

using namespace alia;
using namespace alia::operators;

alia_msdf_text_engine* the_msdf_text_engine = nullptr;

#include "prototyping/msdf.h"
#include "prototyping/panel.h"

void
update();

char const* const notargs_vert_src = R"(
layout(location = 0) in vec2 position;
void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

char const* const notargs_frag_src = R"(
out vec4 frag_color;

uniform vec2 corner;
uniform vec2 size;
uniform float zoom;

uniform float t;

uniform float curl;
uniform float thickness;
uniform float cosx_factor;
uniform float cosy_factor;
uniform float siny_factor;
uniform float sinz_factor;

uniform bool normalize_rays;
uniform float step_scaling;
uniform int iterations;

uniform bool invert;
uniform vec3 color;
uniform float color_factor;
uniform float sine_factor;
uniform float depth_scale;

const float tau = radians(360.0);

float sdf(vec3 p)
{
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
    vec2 half_size = size / 2.0;
    vec2 uv = (gl_FragCoord.xy - corner - half_size) / (zoom * half_size.y);

    vec3 direction;
    if (normalize_rays)
    {
        direction = normalize(vec3(uv, 1.0)) * step_scaling;
    }
    else
    {
        direction = vec3(uv, step_scaling);
    }

    float distance_traveled = 0.0;
    for (int i = 0; i < iterations; ++i)
    {
        distance_traveled += sdf(direction * distance_traveled);
    }

    vec3 p = vec3(direction * distance_traveled);
    float depth_factor = invert ? (length(p) * depth_scale / 120.0)
                                : (1.0 / (length(p) * depth_scale));
    frag_color = vec4(
        (sin(p) * sine_factor + color * color_factor) * depth_factor, 1.0);
}
)";

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

struct notargs_draw_command
{
    alia_draw_command cmd;
    alia_box region;
};

struct GalleryNotargsGl
{
    alia_ui_system* system = nullptr;
    GLuint program = 0;
    GLint loc_corner = -1;
    GLint loc_size = -1;
    GLint loc_zoom = -1;
    GLint loc_t = -1;
    GLint loc_curl = -1;
    GLint loc_thickness = -1;
    GLint loc_cosx = -1;
    GLint loc_cosy = -1;
    GLint loc_siny = -1;
    GLint loc_sinz = -1;
    GLint loc_normalize_rays = -1;
    GLint loc_step_scaling = -1;
    GLint loc_iterations = -1;
    GLint loc_invert = -1;
    GLint loc_color = -1;
    GLint loc_color_factor = -1;
    GLint loc_sine_factor = -1;
    GLint loc_depth_scale = -1;
    GLuint vao = 0;
    GLuint vbo = 0;
};

notargs_controls the_controls;
double the_notargs_t = 0.0;
int the_seed_index = 0;
bool the_light_theme_flag = true;
bool the_theme_dirty_flag = true;

alia_srgb8 const the_seed_primaries[] = {
    hex_color("#154DCF"),
    hex_color("#6f42c1"),
    hex_color("#a52e45"),
    hex_color("#b0edef"),
};

alia_ui_system* the_system = nullptr;
GLFWwindow* the_window = nullptr;
static alia_glfw_ui_binding the_glfw_ui_binding{};
gl_renderer the_renderer{};
alia_arena the_display_list_arena{};
alia_style the_style = {.spacing = 10.0f};
std::chrono::high_resolution_clock::time_point the_last_anim_time
    = std::chrono::high_resolution_clock::now();

GalleryNotargsGl the_gallery_gl{};
alia_draw_material_id the_notargs_material_id = 0;

void
notargs_gl_init(GalleryNotargsGl* gpu, alia_ui_system* system)
{
    gpu->system = system;
    gpu->program = create_shader_program(notargs_vert_src, notargs_frag_src);
    if (gpu->program == 0)
    {
        std::cerr << "notargs: shader program creation failed\n";
        return;
    }
    gpu->loc_corner = glGetUniformLocation(gpu->program, "corner");
    gpu->loc_size = glGetUniformLocation(gpu->program, "size");
    gpu->loc_zoom = glGetUniformLocation(gpu->program, "zoom");
    gpu->loc_t = glGetUniformLocation(gpu->program, "t");
    gpu->loc_curl = glGetUniformLocation(gpu->program, "curl");
    gpu->loc_thickness = glGetUniformLocation(gpu->program, "thickness");
    gpu->loc_cosx = glGetUniformLocation(gpu->program, "cosx_factor");
    gpu->loc_cosy = glGetUniformLocation(gpu->program, "cosy_factor");
    gpu->loc_siny = glGetUniformLocation(gpu->program, "siny_factor");
    gpu->loc_sinz = glGetUniformLocation(gpu->program, "sinz_factor");
    gpu->loc_normalize_rays
        = glGetUniformLocation(gpu->program, "normalize_rays");
    gpu->loc_step_scaling = glGetUniformLocation(gpu->program, "step_scaling");
    gpu->loc_iterations = glGetUniformLocation(gpu->program, "iterations");
    gpu->loc_invert = glGetUniformLocation(gpu->program, "invert");
    gpu->loc_color = glGetUniformLocation(gpu->program, "color");
    gpu->loc_color_factor = glGetUniformLocation(gpu->program, "color_factor");
    gpu->loc_sine_factor = glGetUniformLocation(gpu->program, "sine_factor");
    gpu->loc_depth_scale = glGetUniformLocation(gpu->program, "depth_scale");

    float vertices[] = {
        -1.0f,
        -1.0f,
        1.0f,
        -1.0f,
        1.0f,
        1.0f,
        -1.0f,
        1.0f,
        -1.0f,
        -1.0f,
        1.0f,
        1.0f};

    glGenVertexArrays(1, &gpu->vao);
    glGenBuffers(1, &gpu->vbo);
    glBindVertexArray(gpu->vao);
    glBindBuffer(GL_ARRAY_BUFFER, gpu->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
notargs_gl_destroy(GalleryNotargsGl* gpu)
{
    if (gpu->vbo)
        glDeleteBuffers(1, &gpu->vbo);
    if (gpu->vao)
        glDeleteVertexArrays(1, &gpu->vao);
    if (gpu->program)
        glDeleteProgram(gpu->program);
    *gpu = {};
}

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

void
render_notargs_bucket(void* user, alia_draw_bucket const* bucket)
{
    auto* gpu = static_cast<GalleryNotargsGl*>(user);
    if (!gpu->program || !gpu->system || bucket->count == 0)
        return;

    alia_vec2f const surface_size
        = alia_vec2i_to_vec2f(alia_ui_surface_get_size(gpu->system));

    GLboolean const blend_was_enabled = glIsEnabled(GL_BLEND);
    glDisable(GL_BLEND);
    glUseProgram(gpu->program);

    glUniform1f(gpu->loc_zoom, float(std::exp(the_controls.zoom)));
    glUniform1f(gpu->loc_t, float(the_notargs_t));
    glUniform1f(gpu->loc_curl, float(the_controls.curl));
    glUniform1f(gpu->loc_thickness, float(the_controls.thickness));
    glUniform1f(gpu->loc_cosx, the_controls.include_cosx ? 1.0f : 0.0f);
    glUniform1f(gpu->loc_cosy, the_controls.include_cosy ? 1.0f : 0.0f);
    glUniform1f(gpu->loc_siny, the_controls.include_siny ? 1.0f : 0.0f);
    glUniform1f(gpu->loc_sinz, the_controls.include_sinz ? 1.0f : 0.0f);
    glUniform1i(gpu->loc_normalize_rays, the_controls.normalize_rays ? 1 : 0);
    glUniform1f(gpu->loc_step_scaling, float(the_controls.step_scaling));
    int const iter
        = (int) std::lround(std::clamp(the_controls.iterations, 0.0, 120.0));
    glUniform1i(gpu->loc_iterations, iter);
    glUniform1i(gpu->loc_invert, the_light_theme_flag ? 1 : 0);

    float rgb[3];
    shader_color_for_seed(the_seed_index, rgb);
    glUniform3f(gpu->loc_color, rgb[0], rgb[1], rgb[2]);
    glUniform1f(gpu->loc_color_factor, float(the_controls.color_factor));
    glUniform1f(gpu->loc_sine_factor, float(the_controls.sine_factor));
    glUniform1f(gpu->loc_depth_scale, float(the_controls.depth_scale));

    glBindVertexArray(gpu->vao);

    for (alia_draw_command* walk = bucket->head; walk != nullptr;
         walk = walk->next)
    {
        auto* nd = reinterpret_cast<notargs_draw_command*>(walk);
        alia_box const& r = nd->region;

        float const corner_x = r.min.x;
        float const corner_y = surface_size.y - (r.min.y + r.size.y);
        float const size_x = r.size.x;
        float const size_y = r.size.y;

        glViewport(
            GLint(corner_x + 0.5f),
            GLint(corner_y + 0.5f),
            GLsizei(size_x + 0.5f),
            GLsizei(size_y + 0.5f));

        float const corner_uv[2] = {corner_x, corner_y};
        float const size_uv[2] = {size_x, size_y};
        glUniform2fv(gpu->loc_corner, 1, corner_uv);
        glUniform2fv(gpu->loc_size, 1, size_uv);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);

    if (blend_was_enabled)
        glEnable(GL_BLEND);
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

template<class Content>
void
with_palette(context& ctx, alia_palette* palette, Content&& content)
{
    alia_palette* old_palette = ctx.palette;
    ctx.palette = palette;
    content();
    ctx.palette = old_palette;
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
do_subheading(context& ctx, char const* text)
{
    do_text(
        ctx,
        2,
        alia_srgba8_from_srgb8(ctx.palette->foundation.text.stronger_1),
        14.f,
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
        alia_element_box_region(&ctx, id, &row_box, ALIA_CURSOR_DEFAULT);
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
        alia_element_box_region(&ctx, id, &row_box, ALIA_CURSOR_DEFAULT);
    });
}

void
section_spacer(context& ctx)
{
    switch (get_event_category(ctx))
    {
        case ALIA_CATEGORY_REFRESH:
            alia_layout_leaf_emit(
                &ctx, {1.f, 28.f}, raw_code(ALIGN_TOP | ALIGN_LEFT));
            break;
        default:
            (void) alia_layout_consume_box(&ctx);
            break;
    }
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
control_switch_d(context& ctx, char const* label, bool* value)
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
do_theme_controls(context& ctx)
{
    concrete_panel(
        ctx, 0, ctx.palette->foundation.background.stronger_2, FILL, [&]() {
            inset(
                ctx,
                {.left = 12, .right = 12, .top = 12, .bottom = 12},
                [&]() {
                    row(ctx, [&]() {
                        {
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
                        }
                        do_text(
                            ctx,
                            2,
                            alia_srgba8_from_srgb8(
                                ctx.palette->foundation.text.base),
                            14.f,
                            " ",
                            CENTER_Y);
                        for (int i = 0; i < 4; ++i)
                        {
                            alia_bool_signal radio{
                                .flags
                                = ALIA_SIGNAL_READABLE | ALIA_SIGNAL_WRITABLE,
                                .value = (the_seed_index == i),
                            };
                            char const* labels[]
                                = {"Blue", "Violet", "Red", "Ice"};
                            do_radio_with_text(ctx, &radio, labels[i]);
                            if (radio.flags & ALIA_SIGNAL_WRITTEN)
                            {
                                the_seed_index = i;
                                the_theme_dirty_flag = true;
                                abort_pass(ctx);
                            }
                        }
                    });
                });
        });
}

void
do_notargs_controls(context& ctx)
{
    do_heading(ctx, "View");
    control_slider_d(ctx, "Zoom", &the_controls.zoom, -3.0, 5.0, 0.01);
    control_switch_d(ctx, "Animate", &the_controls.animate);
    control_slider_d(ctx, "Speed", &the_controls.speed, -2.0, 7.0, 0.1);

    section_spacer(ctx);

    do_heading(ctx, "Ray Marching");
    control_switch_d(ctx, "Normalize Rays", &the_controls.normalize_rays);
    control_slider_d(
        ctx, "Step Scaling", &the_controls.step_scaling, 0.0, 1.0, 0.001);
    control_slider_d(
        ctx, "Iterations", &the_controls.iterations, 0.0, 120.0, 1.0);

    section_spacer(ctx);

    do_heading(ctx, "Shape");
    control_slider_d(ctx, "Curl", &the_controls.curl, 0.0, 1.0, 0.001);
    control_slider_d(
        ctx, "Thickness", &the_controls.thickness, 0.0, 2.0, 0.001);
    control_switch_d(ctx, "Include CosX", &the_controls.include_cosx);
    control_switch_d(ctx, "Include CosY", &the_controls.include_cosy);
    control_switch_d(ctx, "Include SinY", &the_controls.include_siny);
    control_switch_d(ctx, "Include SinZ", &the_controls.include_sinz);

    section_spacer(ctx);

    do_heading(ctx, "Coloring");
    control_slider_d(
        ctx, "Color Factor", &the_controls.color_factor, 0.0, 2.0, 0.001);
    control_slider_d(
        ctx, "Sine Factor", &the_controls.sine_factor, 0.0, 4.0, 0.001);
    control_slider_d(
        ctx, "Depth Scale", &the_controls.depth_scale, 0.0, 4.0, 0.001);
}

void
notargs_view(context& ctx)
{
    column(ctx, GROW, [&]() {
        switch (get_event_category(ctx))
        {
            case ALIA_CATEGORY_REFRESH:
                alia_layout_leaf_emit(
                    &ctx, {40.f, 40.f}, raw_code(GROW | FILL));
                break;
            case ALIA_CATEGORY_DRAWING: {
                alia_box const box = alia_layout_consume_box(&ctx);
                notargs_draw_command* cmd
                    = (notargs_draw_command*) alia_draw_command_alloc(
                        &ctx,
                        0,
                        the_notargs_material_id,
                        sizeof(notargs_draw_command));
                cmd->region = box;
                break;
            }
            default:
                (void) alia_layout_consume_box(&ctx);
                break;
        }
    });
}

void
shader_gallery_root(context& ctx)
{
    try
    {
        if (the_theme_dirty_flag)
        {
            alia_palette_seeds pseeds = alia_seeds_from_elevation(
                the_seed_primaries[the_seed_index], 0, !the_light_theme_flag);
            alia_theme_params params = {
                .foundation_step_l = 0.075f,
                .is_dark_mode = !the_light_theme_flag,
            };
            alia_palette_expand(&the_system->palette, &pseeds, &params);
            the_theme_dirty_flag = false;
        }

        with_spacing(ctx, 0, [&] {
            row(ctx, [&]() {
                concrete_panel(
                    ctx,
                    0,
                    ctx.palette->foundation.background.stronger_2,
                    FILL,
                    [&]() {
                        column(ctx, GROW, [&]() {
                            alia_ui_scroll_view_begin(
                                &ctx, ALIA_GROW, 0x2, 0, nullptr);
                            inset(
                                ctx,
                                {.left = 20,
                                 .right = 20,
                                 .top = 20,
                                 .bottom = 20},
                                [&]() {
                                    with_spacing(ctx, 8, [&] {
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
                    [&]() { notargs_view(ctx); });
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
check_and_update_resolution()
{
#ifdef __EMSCRIPTEN__
    double css_w, css_h;
    emscripten_get_element_css_size("#canvas", &css_w, &css_h);
    double dpr = emscripten_get_device_pixel_ratio();
    int target_w = (int) (css_w * dpr);
    int target_h = (int) (css_h * dpr);
    int current_w, current_h;
    glfwGetWindowSize(the_window, &current_w, &current_h);
    if (current_w != target_w || current_h != target_h)
    {
        glfwSetWindowSize(the_window, target_w, target_h);
        the_system->surface_size = {target_w, target_h};
    }
#endif
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

    alia_rgb c
        = alia_rgb_from_srgb8(the_system->palette.foundation.background.base);
    glClearColor(c.r, c.g, c.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    alia_draw_bucket_table bucket_table = {
        .buckets = {},
        .keys = {},
    };
    alia_draw_context draw_context = {
        .buckets = &bucket_table,
        .arena = {},
    };
    alia_bump_allocator_init(&draw_context.arena, &the_display_list_arena);

    auto draw_event = alia_make_draw_event({.context = &draw_context});
    dispatch_event(*the_system, draw_event);

    std::sort(bucket_table.keys.begin(), bucket_table.keys.end());
    for (auto const key : bucket_table.keys)
    {
        alia_draw_bucket* bucket = &bucket_table.buckets[key];
        alia_draw_material_id material_id = key & 0xffff;
        alia_draw_material* material
            = &the_system->draw.materials[material_id];
        material->vtable.draw_bucket(material->user, bucket);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glfwSwapBuffers(the_window);
}

void
main_loop_step()
{
    if (glfwWindowShouldClose(the_window))
    {
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

    glfwPollEvents();
    check_and_update_resolution();
    update();
}

int
main()
{
    void* ui_system_storage = malloc(alia_ui_system_object_spec().size);
    the_system = alia_ui_system_init(
        ui_system_storage, shader_gallery_root_controller, nullptr, {0, 0});

    the_theme_dirty_flag = true;

    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

#ifndef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    the_window
        = glfwCreateWindow(1200, 800, "Alia Shader Gallery", nullptr, nullptr);
    if (!the_window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    the_glfw_ui_binding.ui = the_system;
    alia_glfw_install_surface_callbacks(the_window, &the_glfw_ui_binding);
    alia_glfw_install_default_input_callbacks(
        the_window, &the_glfw_ui_binding);

    float xscale, yscale;
    glfwGetWindowContentScale(the_window, &xscale, &yscale);
    alia_ui_surface_set_dpi(the_system, ((xscale + yscale) / 2.0f) * 96.f);

#ifndef __EMSCRIPTEN__
    glfwMakeContextCurrent(the_window);
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwTerminate();
        return -1;
    }
#endif

    {
        int x, y;
        glfwGetFramebufferSize(the_window, &x, &y);
        alia_ui_surface_set_size(the_system, {x, y});
    }

#ifndef __EMSCRIPTEN__
    glEnable(GL_FRAMEBUFFER_SRGB);
#endif

    init_gl_renderer(the_system, &the_renderer);
    alia_material_register(
        the_system,
        ALIA_PRIMITIVE_MATERIAL_ID,
        alia_material_vtable{.draw_bucket = render_primitive_command_list},
        &the_renderer);

    the_notargs_material_id = alia_material_alloc_ids(the_system, 1);
    notargs_gl_init(&the_gallery_gl, the_system);
    alia_material_register(
        the_system,
        the_notargs_material_id,
        alia_material_vtable{.draw_bucket = render_notargs_bucket},
        &the_gallery_gl);

    std::vector<std::uint8_t> atlas_rgb(
        static_cast<std::size_t>(alia_atlas_decompressed_size));
    alia_msdf_atlas_rle_decompress(
        alia_atlas_rle_r,
        alia_atlas_rle_r_size,
        alia_atlas_rle_g,
        alia_atlas_rle_g_size,
        alia_atlas_rle_b,
        alia_atlas_rle_b_size,
        atlas_rgb.data(),
        atlas_rgb.size());
    gl_renderer_upload_msdf_atlas(
        &the_renderer, atlas_rgb.data(), alia_atlas_width, alia_atlas_height);
    the_msdf_text_engine = alia_msdf_create_text_engine(
        alia_font_descriptions, alia_font_count);
    the_system->msdf_text_engine = the_msdf_text_engine;

    initialize_lazy_commit_arena(&the_display_list_arena);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    refresh_system(*the_system);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop_step, 0, 1);
#else
    while (!glfwWindowShouldClose(the_window))
        main_loop_step();
#endif

    notargs_gl_destroy(&the_gallery_gl);
    glfwTerminate();
    return 0;
}
