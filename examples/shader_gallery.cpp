#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/core/timing/cubic_bezier.hpp>
#include <alia/core/timing/smoothing.hpp>
#include <alia/ui.hpp>
#include <alia/ui/color.hpp>
#include <alia/ui/events.hpp>
#include <alia/ui/layout/grids.hpp>
#include <alia/ui/layout/simple.hpp>
#include <alia/ui/layout/spacer.hpp>
#include <alia/ui/layout/specification.hpp>
#include <alia/ui/layout/system.hpp>
#include <alia/ui/layout/utilities.hpp>
#include <alia/ui/library/bullets.hpp>
#include <alia/ui/library/checkbox.hpp>
#include <alia/ui/library/collapsible.hpp>
#include <alia/ui/library/lazy_list.hpp>
#include <alia/ui/library/node_expander.hpp>
#include <alia/ui/library/panels.hpp>
#include <alia/ui/library/radio_button.hpp>
#include <alia/ui/library/separator.hpp>
#include <alia/ui/library/slider.hpp>
#include <alia/ui/library/switch.hpp>
#include <alia/ui/scrolling.hpp>
#include <alia/ui/system/api.hpp>
#include <alia/ui/system/input_constants.hpp>
#include <alia/ui/system/object.hpp>
#include <alia/ui/text/fonts.hpp>
#include <alia/ui/utilities/cached_ui.hpp>
#include <alia/ui/utilities/culling.hpp>
#include <alia/ui/utilities/keyboard.hpp>
#include <alia/ui/utilities/mouse.hpp>
#include <alia/ui/utilities/regions.hpp>
#include <alia/ui/utilities/rendering.hpp>
#include <alia/ui/utilities/skia.hpp>
#include <alia/ui/utilities/styling.hpp>

#include <bit>
#include <cmath>
#include <glad/glad.h>
#include <limits>
#include <optional>
#include <vector>

#include <alia/core/flow/components.hpp>
#include <alia/core/flow/macros.hpp>
#include <alia/core/flow/top_level.hpp>
#include <alia/core/signals/core.hpp>
#include <alia/core/timing/smoothing.hpp>
#include <alia/core/timing/ticks.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/geometry.hpp>
#include <alia/ui/layout/internals.hpp>
#include <alia/ui/text/display.hpp>
#include <alia/ui/utilities/animation.hpp>
#include <alia/ui/utilities/click_flares.hpp>

#ifdef _WIN32
#pragma warning(push, 0)
#endif

#include <include/core/SkBlurTypes.h>
#include <include/core/SkColor.h>
#include <include/core/SkFontTypes.h>
#include <include/core/SkMaskFilter.h>
#include <include/core/SkPath.h>

#ifdef _WIN32
#pragma warning(pop)
#endif

#include "assets.hpp"

using namespace alia;

color_palette the_palette;

seed_colors const seed_sets[] = {
    {.primary = hex_color("#154DCF"),
     .secondary = hex_color("#6C36AE"),
     .tertiary = hex_color("#E01D23"),
     .neutral = hex_color("#1f212a"),
     .warning = hex_color("#FF9D00"),
     .danger = hex_color("#E01D23")},
    {.primary = hex_color("#6f42c1"),
     .secondary = hex_color("#7d8bae"),
     .tertiary = hex_color("#f1b2b2"),
     .neutral = hex_color("#1f212a"),
     .warning = hex_color("#e5857b"),
     .danger = hex_color("#d31638")},
    {.primary = hex_color("#a52e45"),
     .secondary = hex_color("#2b5278"),
     .tertiary = hex_color("#61787b"),
     .neutral = hex_color("#1f212a"),
     .warning = hex_color("#ead8b1"),
     .danger = hex_color("#d31638")},
    {.primary = hex_color("#b0edef"),
     .secondary = hex_color("#505888"),
     .tertiary = hex_color("#e1b7c5"),
     .neutral = hex_color("#1f212a"),
     .warning = hex_color("#ded0d1"),
     .danger = hex_color("#d31638")},
};

seed_colors const* seeds = &seed_sets[0];

bool light_theme = true;

bool theme_update_needed = true;

using cubic_bezier = unit_cubic_bezier;

std::vector<std::string>
split_string(char const* str)
{
    std::vector<std::string> ret;
    char const* line_start = str;
    char const* p = str;
    for (; *p; ++p)
    {
        if (*p == '\r' || *p == '\n')
        {
            if (line_start != p)
                ret.push_back(std::string(line_start, p - line_start));
            line_start = p + 1;
        }
    }
    if (line_start != p)
        ret.push_back(std::string(line_start, p - line_start));
    return ret;
}

void
do_code_snippet(ui_context ctx, std::vector<std::string> const& snippets)
{
    panel_style_info style{
        .margin = box_border_width<float>{4, 4, 4, 4},
        .border_width = box_border_width<float>{0, 0, 0, 0},
        .padding = box_border_width<float>{8, 8, 8, 8},
        .background_color = get_system(ctx).theme.background.stronger[0].main};
    panel p(ctx, direct(style), layout(size(100, 20), FILL));

    auto my_style = style_info{
        font_info{&get_font(
            "Roboto_Mono/static/RobotoMono-Regular",
            20.f * get_system(ctx).magnification)},
        get_system(ctx).theme.foreground.base.main};
    scoped_style_info scoped_style(ctx, my_style);

    for (auto const& snippet : snippets)
    {
        do_text(ctx, value(snippet));
    }
}

void
check_gl_errors()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        const char* error_msg;
        switch (error)
        {
            case GL_INVALID_ENUM:
                error_msg = "GL_INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error_msg = "GL_INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error_msg = "GL_INVALID_OPERATION";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error_msg = "GL_INVALID_FRAMEBUFFER_OPERATION";
                break;
            case GL_OUT_OF_MEMORY:
                error_msg = "GL_OUT_OF_MEMORY";
                break;
            default:
                error_msg = "Unknown error";
        }
        assert(false && error_msg);
    }
}

millisecond_count
get_animation_delta(ui_context ctx)
{
    millisecond_count* last_tick_count;
    auto now = get_raw_animation_tick_count(ctx);
    if (get_cached_data(ctx, &last_tick_count))
    {
        *last_tick_count = now;
    }
    auto const delta = now - *last_tick_count;
    *last_tick_count = now;
    return delta;
}

namespace alia {

void
reset_skia_context();

void
toggle_full_screen();

} // namespace alia

struct notargs_graphics_data
{
    bool initialized = false;
    layout_leaf layout_node;
    GLuint fragment_shader;
    GLuint vertex_shader;
    GLuint shader_program;

    GLint corner_loc;
    GLint size_loc;
    GLint zoom_loc;

    GLint t_loc;

    GLint curl_loc;
    GLint thickness_loc;
    GLint cosx_factor_loc;
    GLint cosy_factor_loc;
    GLint siny_factor_loc;
    GLint sinz_factor_loc;

    GLint normalize_rays_loc;
    GLint step_scaling_loc;
    GLint iterations_loc;

    GLint depth_scale_loc;
    GLint color_loc;
    GLint color_factor_loc;
    GLint sine_factor_loc;
    GLint invert_loc;
};

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
    int iterations = 32;

    double color_factor = 1.0;
    double sine_factor = 1.0;
    double depth_scale = 1.0;
};

void
notargs_graphics(
    ui_context ctx,
    layout const& layout_spec,
    notargs_controls const& controls)
{
    static bool glad_initialized = false;
    if (!glad_initialized)
    {
        gladLoadGL();
        glad_initialized = true;
    }

    auto& data = get_cached_data<notargs_graphics_data>(ctx);

    double* t;
    if (get_cached_data(ctx, &t))
        *t = 0;
    auto const delta = get_animation_delta(ctx);
    if (controls.animate)
        *t += double(delta) / 1000.f * std::exp(double(controls.speed)) * 0.1f;

    switch (get_event_category(ctx))
    {
        case REFRESH_CATEGORY:
            data.layout_node.refresh_layout(
                get_layout_traversal(ctx),
                layout_spec,
                leaf_layout_requirements(make_layout_vector(40, 40), 0, 0),
                LEFT | BASELINE_Y | PADDED);

            add_layout_node(
                get<ui_traversal_tag>(ctx).layout, &data.layout_node);

            break;

        case RENDER_CATEGORY:
            // glPopClientAttrib();
            // glPopAttrib();

            glViewport(
                GLsizei(data.layout_node.assignment().region.corner[0] + 0.5),
                GLsizei(data.layout_node.assignment().region.corner[1] + 0.5),
                GLsizei(data.layout_node.assignment().region.size[0] + 0.5),
                GLsizei(data.layout_node.assignment().region.size[1] + 0.5));

            if (!data.initialized)
            {
                auto const fragment_shader_asset = load_asset("notargs.glsl");
                std::string fragment_shader_text(
                    reinterpret_cast<char*>(fragment_shader_asset.data.get()),
                    fragment_shader_asset.size);
                char const* fragment_shader_pointer
                    = fragment_shader_text.c_str();

                static const char* vertex_shader_text = R"(
                in vec2 position;
                void main()
                {
                    gl_Position = vec4(position, 0, 1);
                }
                )";

                // Create and compile shaders
                data.fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
                glShaderSource(
                    data.fragment_shader,
                    1,
                    &fragment_shader_pointer,
                    nullptr);
                glCompileShader(data.fragment_shader);
                GLint success;
                GLchar info_log[512];
                glGetShaderiv(
                    data.fragment_shader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    glGetShaderInfoLog(
                        data.fragment_shader, 512, NULL, info_log);
                    throw alia::exception(
                        std::string("Fragment shader compilation failed: ")
                        + info_log);
                }

                data.vertex_shader = glCreateShader(GL_VERTEX_SHADER);
                glShaderSource(
                    data.vertex_shader, 1, &vertex_shader_text, nullptr);
                glCompileShader(data.vertex_shader);

                // Create and link program
                data.shader_program = glCreateProgram();
                glAttachShader(data.shader_program, data.fragment_shader);
                glAttachShader(data.shader_program, data.vertex_shader);
                glLinkProgram(data.shader_program);

                data.corner_loc
                    = glGetUniformLocation(data.shader_program, "corner");
                data.size_loc
                    = glGetUniformLocation(data.shader_program, "size");
                data.zoom_loc
                    = glGetUniformLocation(data.shader_program, "zoom");

                data.t_loc = glGetUniformLocation(data.shader_program, "t");

                data.curl_loc
                    = glGetUniformLocation(data.shader_program, "curl");
                data.thickness_loc
                    = glGetUniformLocation(data.shader_program, "thickness");
                data.cosx_factor_loc
                    = glGetUniformLocation(data.shader_program, "cosx_factor");
                data.cosy_factor_loc
                    = glGetUniformLocation(data.shader_program, "cosy_factor");
                data.siny_factor_loc
                    = glGetUniformLocation(data.shader_program, "siny_factor");
                data.sinz_factor_loc
                    = glGetUniformLocation(data.shader_program, "sinz_factor");

                data.normalize_rays_loc = glGetUniformLocation(
                    data.shader_program, "normalize_rays");
                data.step_scaling_loc = glGetUniformLocation(
                    data.shader_program, "step_scaling");
                data.iterations_loc
                    = glGetUniformLocation(data.shader_program, "iterations");

                data.invert_loc
                    = glGetUniformLocation(data.shader_program, "invert");
                data.color_loc
                    = glGetUniformLocation(data.shader_program, "color");
                data.color_factor_loc = glGetUniformLocation(
                    data.shader_program, "color_factor");
                data.sine_factor_loc
                    = glGetUniformLocation(data.shader_program, "sine_factor");
                data.depth_scale_loc
                    = glGetUniformLocation(data.shader_program, "depth_scale");

                check_gl_errors();

                data.initialized = true;
            }

            // Create quad vertices
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

            GLuint VAO;
            glGenVertexArrays(1, &VAO);
            glBindVertexArray(VAO);

            GLuint VBO;
            glGenBuffers(1, &VBO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(
                GL_ARRAY_BUFFER, sizeof(float) * 12, vertices, GL_STATIC_DRAW);

            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(0);

            glUseProgram(data.shader_program);

            {
                float values[2]
                    = {data.layout_node.assignment().region.size[0],
                       data.layout_node.assignment().region.size[1]};
                glUniform2fv(data.size_loc, 1, values);
            }
            {
                float values[2]
                    = {data.layout_node.assignment().region.corner[0],
                       data.layout_node.assignment().region.corner[1]};
                glUniform2fv(data.corner_loc, 1, values);
            }
            glUniform1f(data.zoom_loc, float(std::exp(controls.zoom)));

            glUniform1f(data.t_loc, float(*t));

            glUniform1f(data.curl_loc, float(controls.curl));
            glUniform1f(data.thickness_loc, float(controls.thickness));
            glUniform1f(
                data.cosx_factor_loc, controls.include_cosx ? 1.0f : 0.0f);
            glUniform1f(
                data.cosy_factor_loc, controls.include_cosy ? 1.0f : 0.0f);
            glUniform1f(
                data.siny_factor_loc, controls.include_siny ? 1.0f : 0.0f);
            glUniform1f(
                data.sinz_factor_loc, controls.include_sinz ? 1.0f : 0.0f);

            glUniform1i(data.normalize_rays_loc, controls.normalize_rays);
            glUniform1f(data.step_scaling_loc, float(controls.step_scaling));
            glUniform1i(data.iterations_loc, controls.iterations);

            switch (seeds - &seed_sets[0])
            {
                case 0:
                default: {
                    float values[3] = {2, 5, 11};
                    glUniform3fv(data.color_loc, 1, values);
                    break;
                }
                case 1: {
                    float values[3] = {5, 2, 7};
                    glUniform3fv(data.color_loc, 1, values);
                    break;
                }
                case 2: {
                    float values[3] = {9, 4, 2};
                    glUniform3fv(data.color_loc, 1, values);
                    break;
                }
            }

            glUniform1i(data.invert_loc, light_theme);
            glUniform1f(data.color_factor_loc, float(controls.color_factor));
            glUniform1f(data.sine_factor_loc, float(controls.sine_factor));
            glUniform1f(data.depth_scale_loc, float(controls.depth_scale));

            glBindVertexArray(VAO);
            glDisable(GL_BLEND);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBindVertexArray(0);

            glDeleteBuffers(1, &VBO);
            glDeleteVertexArrays(1, &VAO);

            check_gl_errors();

            glFlush();

            // glPushAttrib(GL_ALL_ATTRIB_BITS);
            // glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

            reset_skia_context();

            // TODO: Cleanup
            // glDeleteProgram(shader_program);
            // glDeleteShader(fragment_shader);
            // glDeleteShader(vertex_shader);
    }
}

void
do_section_divider(ui_context ctx)
{
    do_spacer(ctx, height(40));
    do_separator(ctx);
    do_spacer(ctx, height(40));
}

void
do_heading(ui_context ctx, char const* text)
{
    auto heading_style = style_info{
        font_info{&get_font(
            "Roboto/Roboto-Bold", 32.f * get_system(ctx).magnification)},
        get_system(ctx).theme.foreground.stronger[1].main};
    scoped_style_info scoped_style(ctx, heading_style);
    do_text(ctx, value(text));
    do_spacer(ctx, height(20));
}

void
do_theme_controls(ui_context ctx)
{
    panel_style_info style{
        .margin = box_border_width<float>{0, 0, 0, 0},
        .border_width = box_border_width<float>{0, 0, 0, 0},
        .padding = box_border_width<float>{8, 8, 8, 8},
        .background_color = lerp(
            get_system(ctx).theme.background.stronger[0].main,
            get_system(ctx).theme.background.base.main,
            0.5)};
    panel p(ctx, direct(style), FILL | UNPADDED);

    bordered_layout bordered(
        ctx,
        box_border_width<absolute_length>{
            absolute_length(16),
            absolute_length(16),
            absolute_length(16),
            absolute_length(16)},
        GROW);

    row_layout row(ctx);

    {
        {
            do_switch(
                ctx, add_write_action(direct(light_theme), callback([&](bool) {
                                          theme_update_needed = true;
                                      })));
            do_text(ctx, value("Light"));
        }

        do_spacer(ctx, width(10));
        do_separator(ctx);
        do_spacer(ctx, width(10));

        {
            {
                do_radio_button(
                    ctx,
                    add_write_action(
                        make_radio_signal(direct(seeds), value(&seed_sets[0])),
                        callback([&](bool) { theme_update_needed = true; })),
                    "Blue");
            }

            do_spacer(ctx, width(10));
            do_separator(ctx);
            do_spacer(ctx, width(10));

            {
                do_radio_button(
                    ctx,
                    add_write_action(
                        make_radio_signal(direct(seeds), value(&seed_sets[1])),
                        callback([&](bool) { theme_update_needed = true; })),
                    "Violet");
            }

            do_spacer(ctx, width(10));
            do_separator(ctx);
            do_spacer(ctx, width(10));

            {
                do_radio_button(
                    ctx,
                    add_write_action(
                        make_radio_signal(direct(seeds), value(&seed_sets[2])),
                        callback([&](bool) { theme_update_needed = true; })),
                    "Red");
            }
        }
    }
}

void
control_slider(
    ui_context ctx,
    grid_layout& grid,
    char const* label,
    duplex<double> value,
    double min,
    double max,
    double step)
{
    grid_row row(grid);
    do_text(ctx, alia::value(label), CENTER_Y);
    do_spacer(ctx, layout(width(20), GROW));
    do_text(ctx, alia::printf(ctx, "%.3f", value));
    do_spacer(ctx, layout(width(20), GROW));
    do_slider(ctx, value, min, max, step, layout(width(420), CENTER_Y));
}

void
control_switch(
    ui_context ctx, grid_layout& grid, char const* label, duplex<bool> value)
{
    grid_row row(grid);
    do_text(ctx, alia::value(label), CENTER_Y);
    do_spacer(ctx, layout(width(20), GROW));
    do_switch(ctx, value);
}

void
do_notargs_controls(ui_context ctx, duplex<notargs_controls> controls)
{
    grid_layout grid(ctx);

    do_heading(ctx, "View");

    control_slider(
        ctx, grid, "Zoom", alia_field(controls, zoom), -3.0, 5.0, 0.01);

    control_switch(ctx, grid, "Animate", alia_field(controls, animate));

    control_slider(
        ctx, grid, "Speed", alia_field(controls, speed), -2.0, 7.0, 0.1);

    do_section_divider(ctx);

    do_heading(ctx, "Ray Marching");

    control_switch(
        ctx, grid, "Normalize Rays", alia_field(controls, normalize_rays));
    control_slider(
        ctx,
        grid,
        "Step Scaling",
        alia_field(controls, step_scaling),
        0.0,
        1.0,
        0.001);

    control_slider(
        ctx,
        grid,
        "Iterations",
        signal_cast<double>(alia_field(controls, iterations)),
        0,
        120,
        1);

    do_section_divider(ctx);

    do_heading(ctx, "Shape");

    control_slider(
        ctx, grid, "Curl", alia_field(controls, curl), 0.0, 1.0, 0.001);
    control_slider(
        ctx,
        grid,
        "Thickness",
        alia_field(controls, thickness),
        0.0,
        2.0,
        0.001);
    control_switch(
        ctx, grid, "Include CosX", alia_field(controls, include_cosx));
    control_switch(
        ctx, grid, "Include CosY", alia_field(controls, include_cosy));
    control_switch(
        ctx, grid, "Include SinY", alia_field(controls, include_siny));
    control_switch(
        ctx, grid, "Include SinZ", alia_field(controls, include_sinz));

    do_section_divider(ctx);
    do_heading(ctx, "Coloring");

    control_slider(
        ctx,
        grid,
        "Color Factor",
        alia_field(controls, color_factor),
        0.0,
        2.0,
        0.001);
    control_slider(
        ctx,
        grid,
        "Sine Factor",
        alia_field(controls, sine_factor),
        0.0,
        4.0,
        0.001);
    control_slider(
        ctx,
        grid,
        "Depth Scale",
        alia_field(controls, depth_scale),
        0.0,
        4.0,
        0.001);
}

void
my_ui(ui_context ctx)
{
    // F11 toggles full screen mode.
    // if (detect_key_press(ctx, key_code::F11))
    // {
    //     toggle_full_screen();
    //     abort_traversal(ctx);
    // }

    if (detect_key_press(ctx, key_code::MINUS, KMOD_CTRL))
    {
        get_system(ctx).magnification /= 1.1f;
        abort_traversal(ctx);
    }

    if (detect_key_press(ctx, key_code::EQUAL, KMOD_CTRL))
    {
        get_system(ctx).magnification *= 1.1f;
        abort_traversal(ctx);
    }

    if (theme_update_needed)
    {
        the_palette = generate_color_palette(*seeds);

        contrast_parameters contrast;
        contrast.light_on_dark_ratio = 6;
        contrast.dark_on_light_ratio = 8;

        theme_colors theme;
        theme = generate_theme_colors(
            light_theme ? ui_lightness_mode::LIGHT_MODE
                        : ui_lightness_mode::DARK_MODE,
            *seeds,
            contrast);
        get_system(ctx).theme = theme;

        theme_update_needed = false;
    }

    auto const& theme = get_system(ctx).theme;

    auto main_style = style_info{
        font_info{&get_font(
            "Roboto/Roboto-Regular", 22.f * get_system(ctx).magnification)},
        theme.foreground.base.main};
    scoped_style_info main_scoped_style(ctx, main_style);

    row_layout root_row(ctx, GROW);

    auto controls
        = get_state(ctx, lambda_constant([] { return notargs_controls{}; }));

    {
        column_layout controls_column(ctx);

        panel background_panel(
            ctx,
            direct(panel_style_info{
                .margin = box_border_width<float>{0, 0, 0, 0},
                .border_width = box_border_width<float>{0, 0, 0, 0},
                .padding = box_border_width<float>{0, 0, 0, 0},
                .background_color
                = rgba8(get_system(ctx).theme.background.base.main, 0xf0)}),
            GROW | UNPADDED);

        {
            scoped_scrollable_view scrollable(ctx, GROW, 2);

            bordered_layout bordered(
                ctx,
                box_border_width<absolute_length>{
                    absolute_length(20),
                    absolute_length(20),
                    absolute_length(20),
                    absolute_length(20)});

            column_layout column(ctx);

            do_notargs_controls(ctx, controls);
        }

        do_theme_controls(ctx);
    }

    notargs_graphics(ctx, GROW | UNPADDED, read_signal(controls));
}
