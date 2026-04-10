#pragma once

#if 0

void
rectangle_demo(context& ctx)
{
    static bool invert = false;

    with_spacing(ctx, 0, [&] {
        column(ctx, [&]() {
            flow(ctx, [&]() {
                float x = 0.0f;
                for (int i = 0; i < 10; ++i)
                {
                    flow(ctx, [&]() {
                        alia_z_index const rect_z_index = 0;
                        alia_z_index const text_z_index = 1;

                        for (int j = 0; j < 500; ++j)
                        {
                            float f = fmod(x, 1.0f);
                            if (do_rect(
                                    ctx,
                                    rect_z_index,
                                    {24, 24},
                                    invert ? rgba{f, 0.1f, 1.0f - f, 1}
                                           : rgba{1.0f - f, 0.1f, f, 1},
                                    ALIGN_TOP | ALIGN_LEFT))
                            {
                                invert = !invert;
                                return;
                            }
                            x += 0.0015f;
                        }

                        with_spacing(ctx, 20, [&] {
                            for (int j = 0; j < 1; ++j)
                            {
                                do_text(
                                    ctx,
                                    text_z_index,
                                    ctx.palette->foundation.text.base,
                                    24 + i * 6 + j * 4,
                                    "lorem ipsum");
                                if (do_text(
                                        ctx,
                                        text_z_index,
                                        ctx.palette->foundation.text.base,
                                        20 + i * 12 + j * 4,
                                        lorem_ipsum))
                                {
                                    invert = !invert;
                                    return;
                                }
                            }
                        });
                    });
                }
            });
        });
    });
}

void
text_demo(context& ctx)
{
    static bool invert = false;

    inset(ctx, {.left = 10, .right = 10, .top = 10, .bottom = 10}, [&]() {
        column(ctx, [&]() {
            for (int i = 0; i < 10; ++i)
            {
                with_spacing(ctx, 8, [&] {
                    row(ctx, [&]() {
                        do_text(ctx, 1, GRAY, 40, "test");
                        flow(ctx, GROW, [&]() {
                            for (int j = 0; j < 10; ++j)
                            {
                                flow(ctx, [&]() {
                                    do_text(
                                        ctx, 1, GRAY, 10 + i * 6, lorem_ipsum);
                                });
                                do_text(ctx, 1, GRAY, 16 + i * 4, "lorum");
                                do_text(ctx, 1, GRAY, 20 + i * 2, "ipsum");
                            }
                        });
                    });
                });
            }
        });
    });
}

void
simple_text_demo(context& ctx)
{
    static bool invert = false;

    inset(ctx, {.left = 10, .right = 10, .top = 10, .bottom = 10}, [&]() {
        column(ctx, [&]() {
            for (int i = 0; i < 12; ++i)
            {
                do_text(
                    ctx,
                    1,
                    GRAY,
                    20 + (10 - i) * 4,
                    " !\"#$%&'()*+,-./0123456789:;<=>?@AZaz[]^_`{|}~");
            }
        });
    });
}

void
nested_flow_demo(context& ctx)
{
    static bool invert = false;

    inset(ctx, {.left = 100, .right = 100, .top = 100, .bottom = 100}, [&]() {
        flow(ctx, [&]() {
            for (int i = 0; i < 10; ++i)
            {
                flow(ctx, [&]() {
                    do_text(ctx, 1, GRAY, 10 + i * 6, lorem_ipsum);
                });
                do_text(ctx, 1, GRAY, 16 + i * 4, "lorum");
                do_text(ctx, 1, GRAY, 20 + i * 2, "ipsum");
            }
        });
    });
}

#endif

void
mixed_flow_demo(alia_context& ctx)
{
    with_spacing(ctx, 10, [&] {
        flow(ctx, JUSTIFY_SPACE_BETWEEN, [&]() {
            for (int i = 0; i < 10; ++i)
            {
                float x = 0.0f;
                for (int j = 0; j < 10; ++j)
                {
                    float f = fmod(x, 1.0f);
                    do_rect(
                        ctx,
                        0,
                        {72, 72},
                        alia_srgb8{
                            uint8_t(0xff * f),
                            uint8_t(0xff * 0.1f),
                            uint8_t(0xff * (1.0f - f))},
                        CENTER);
                    x += 0.1f;
                }

                // panel(
                //     ctx,
                //     1,
                //     alia_srgb8{
                //         uint8_t(0xff * 0.05f),
                //         uint8_t(0xff * 0.05f),
                //         uint8_t(0xff * 0.06f)},
                //     min_size({0, 0})
                //         | margins(
                //             {.left = 10,
                //              .right = 10,
                //              .top = 10,
                //              .bottom = 10}),
                //     [&] {
                //         do_text(
                //             ctx,
                //             2,
                //             alia_srgba8_from_srgb8(
                //                 ctx.palette->foundation.text.base),
                //             12 + i * 6,
                //             "panel",
                //             BASELINE_Y);
                //     });
                do_text(
                    ctx,
                    1,
                    alia_srgba8_from_srgb8(ctx.palette->foundation.text.base),
                    12 + i * 4,
                    lorem_ipsum,
                    BASELINE_Y);
            }
        });
    });
}

void
block_flow_demo(alia_context& ctx)
{
    alia::layout_flag_set const justification_flags[6] = {
        JUSTIFY_START,
        JUSTIFY_END,
        JUSTIFY_CENTER,
        JUSTIFY_SPACE_BETWEEN,
        JUSTIFY_SPACE_EVENLY,
        JUSTIFY_SPACE_AROUND,
    };

    with_spacing(ctx, 10, [&] {
        for (int i = 0; i < 6; ++i)
        {
            block_flow(ctx, justification_flags[i], [&]() {
                for (int i = 0; i < 5; ++i)
                {
                    float x = 0.0f;
                    for (int j = 0; j < 20; ++j)
                    {
                        do_rect(
                            ctx,
                            0,
                            {72 + float(j % 3) * 20, 72},
                            alia_srgb8{
                                uint8_t(0xff * x),
                                uint8_t(0xff * 0.1f),
                                uint8_t(0xff * (1.0f - x))},
                            (!(j & 7) ? (GROW | FILL) : CENTER));
                        x += 0.05f;
                    }
                }
            });
        }
    });
}

#if 0

void
layout_demo_flow(context& ctx)
{
    float x = 0.0f;
    block_flow(ctx, [&]() {
        for (int i = 0; i < 600; ++i)
        {
            float intensity = ((i / 4) % 3) * 0.01f;
            inset(
                ctx,
                {.left = 10, .right = 10, .top = 10, .bottom = 10},
                [&]() {
                    concrete_panel(
                        ctx,
                        0,
                        rgba{
                            0.03f + intensity,
                            0.03f + intensity,
                            0.04f + intensity,
                            1},
                        NO_FLAGS,
                        [&]() {
                            min_size_constraint(ctx, {0, 200}, [&]() {
                                row(ctx, [&]() {
                                    float f = fmod(x, 1.0f);
                                    do_rect(
                                        ctx,
                                        1,
                                        {24, float((i & 7) * 12 + 12)},
                                        rgba{f, 0.1f, 1.0f - f, 1},
                                        layout_flag_set(
                                            (i & 3)
                                            << ALIA_CROSS_ALIGNMENT_BIT_OFFSET));
                                    x += 0.01f;
                                });
                            });
                        });
                });
        }
    });
}

void
layout_growth_demo(context& ctx)
{
    float x = 0.0f;
    row(ctx, [&]() {
        for (int i = 0; i < 12; ++i)
        {
            float f = fmod(x, 1.0f);
            growth_override(ctx, i * 1.0f, [&]() {
                do_rect(
                    ctx,
                    0,
                    {6, 12},
                    rgba{f, 0.1f, 1.0f - f, 1},
                    FILL | (i & 1 ? GROW : NO_FLAGS));
            });
            x += 0.08f;
        }
    });
}

void
alignment_override_demo(context& ctx)
{
    static bool invert = false;
    static bool initialized[12] = {false};
    static alia_vec2f offsets[12] = {0};
    float x = 0.0f;
    row(ctx, [&]() {
        concrete_panel(ctx, 0, rgba{0.03f, 0.03f, 0.04f, 1}, CENTER, [&]() {
            inset(ctx, {.left = 4, .right = 4, .top = 4, .bottom = 4}, [&]() {
                if (do_rect(
                        ctx,
                        1,
                        {24, 24},
                        invert ? rgba{1, 1, 1, 1} : rgba{0, 0, 0, 0},
                        CENTER))
                {
                    invert = !invert;
                }
            });
        });
        for (int i = 0; i < 12; ++i)
        {
            float f = fmod(x, 1.0f);
            inset(
                ctx,
                {.left = 10, .right = 10, .top = 10, .bottom = 10},
                [&]() {
                    min_size_constraint(ctx, {0, 200}, [&]() {
                        concrete_panel(
                            ctx,
                            0,
                            rgba{
                                0.03f + 0.02f * f,
                                0.03f + 0.02f * f,
                                0.04f + 0.02f * f,
                                1},
                            NO_FLAGS,
                            [&]() {
                                do_animated_rect(
                                    ctx,
                                    1,
                                    initialized[i],
                                    offsets[i],
                                    {24, 24},
                                    rgba{f, 0.1f, 1.0f - f, 1},
                                    (i & 1) == (invert ? 0 : 1)
                                        ? ALIGN_TOP
                                        : ALIGN_BOTTOM);
                            });
                    });
                });
            x += 0.08f;
        }
    });
}

void
layout_mods_demo(context& ctx)
{
    float x = 0.0f;
    flow(ctx, [&]() {
        for (int i = 0; i < 36; ++i)
        {
            float f = fmod(x, 1.0f);
            panel(
                ctx,
                0,
                rgba{
                    0.03f + 0.02f * f,
                    0.03f + 0.02f * f,
                    0.04f + 0.02f * f,
                    1},
                min_size({120, 120})
                    | margins(
                        {.left = 10, .right = 10, .top = 10, .bottom = 10}),
                [&]() {
                    do_rect(
                        ctx,
                        1,
                        {float(4 * i), float(4 * i)},
                        rgba{f, 0.1f, 1.0f - f, 1},
                        align_right | center_y);
                });
            x += 0.2f;
        }
    });
}

void
grid_demo(context& ctx)
{
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    grid(ctx, [&](auto* grid) {
        float x = 0.0f;
        for (int i = 0; i < 40; ++i)
        {
            grid_row(ctx, grid, [&]() {
                for (int j = 0; j < 40; ++j)
                {
                    float const f = fmod(x, 1.0f);
                    float const size = std::pow(dist(rng), 12.0f) * 80 + 20;
                    do_rect(
                        ctx,
                        0,
                        {size, size},
                        rgba{f, 0.1f, 1.0f - f, 1},
                        CENTER);
                    x += 0.2f;
                }
            });
        }
    });
}

void
layout_demo(context& ctx)
{
    with_spacing(ctx, 10, [&] {
        inset(ctx, {.left = 40, .right = 40, .top = 40, .bottom = 40}, [&]() {
            row(ctx, [&]() {
                float x = 0.0f;
                inset(
                    ctx,
                    {.left = 0, .right = 12, .top = 0, .bottom = 0},
                    [&]() {
                        column(ctx, [&]() {
                            for (int j = 0; j < 80; ++j)
                            {
                                float f = fmod(x, 1.0f);
                                do_rect(
                                    ctx,
                                    0,
                                    {float((j & 7) * 12 + 12), 24},
                                    rgba{f, 0.1f, 1.0f - f, 1},
                                    layout_flag_set(
                                        (j & 3)
                                        << ALIA_X_ALIGNMENT_BIT_OFFSET));
                                x += 0.02f;
                            }
                        });
                    });
                column(ctx, GROW, [&]() {
                    do_text(ctx, 1, GRAY, 24, "layout_growth_demo");
                    layout_growth_demo(ctx);
                    do_text(ctx, 1, GRAY, 24, "alignment_override_demo");
                    alignment_override_demo(ctx);
                    do_text(ctx, 1, GRAY, 24, "layout_mods_demo");
                    layout_mods_demo(ctx);
                    do_text(ctx, 1, GRAY, 24, "mixed_flow_demo");
                    mixed_flow_demo(ctx);
                    do_text(ctx, 1, GRAY, 24, "layout_demo_flow");
                    layout_demo_flow(ctx);
                });
            });
        });
    });
}

static void
show_srgb8_rect(context& ctx, alia_srgb8 c)
{
    do_rect(
        ctx, 1, {36, 36}, alia_rgba_from_rgb(alia_rgb_from_srgb8(c)), CENTER);
}

static void
show_foundation_ramp(context& ctx, alia_foundation_ramp const* ramp)
{
    row(ctx, [&]() {
        show_srgb8_rect(ctx, ramp->weaker_4);
        show_srgb8_rect(ctx, ramp->weaker_3);
        show_srgb8_rect(ctx, ramp->weaker_2);
        show_srgb8_rect(ctx, ramp->weaker_1);
        show_srgb8_rect(ctx, ramp->base);
        show_srgb8_rect(ctx, ramp->stronger_1);
        show_srgb8_rect(ctx, ramp->stronger_2);
        show_srgb8_rect(ctx, ramp->stronger_3);
        show_srgb8_rect(ctx, ramp->stronger_4);
    });
}

static void
show_palette_swatch(context& ctx, alia_swatch const* swatch)
{
    row(ctx, [&]() {
        show_srgb8_rect(ctx, swatch->solid);
        show_srgb8_rect(ctx, swatch->subtle);
        show_srgb8_rect(ctx, swatch->outline);
        show_srgb8_rect(ctx, swatch->text);
    });
}

void
color_ramp(context& ctx, alia_srgb8 seed)
{
    column(ctx, [&]() {
        alia_oklch oklch = alia_oklch_from_oklab(
            alia_oklab_from_rgb(alia_rgb_from_srgb8(seed)));
        for (int i = 0; i < 11; ++i)
        {
            oklch.l = i * 0.1f;
            alia_rgb rgb = alia_rgb_from_oklab(alia_oklab_from_oklch(oklch));
            do_rect(ctx, 1, {36, 36}, alia_rgba_from_rgb(rgb), CENTER);
        }
    });
}

void
color_transition(context& ctx, alia_oklch start, alia_oklch end)
{
    column(ctx, [&]() {
        alia_oklch oklch = start;
        for (int i = 0; i < 101; ++i)
        {
            oklch.l = start.l + (end.l - start.l) * i * 0.01f;
            oklch.c = start.c + (end.c - start.c) * i * 0.01f;
            oklch.h = start.h + (end.h - start.h) * i * 0.01f;
            alia_rgb rgb = alia_rgb_from_oklab(alia_oklab_from_oklch(oklch));
            do_rect(ctx, 1, {36, 36}, alia_rgba_from_rgb(rgb), CENTER);
        }
    });
}

void
color_demo(context& ctx)
{
    block_flow(ctx, [&]() {
        alia_oklch oklch = {.l = 0.7f, .c = 0.2f, .h = 0.0f};
        for (int i = 0; i < 101; ++i)
        {
            oklch.h = i * 0.01f * 2 * 3.14159f;
            alia_rgb rgb = alia_rgb_from_oklab(alia_oklab_from_oklch(oklch));
            with_spacing(ctx, 5, [&] {
                do_rect(ctx, 1, {24, 24}, alia_rgba_from_rgb(rgb), CENTER);
            });
        }
    });
    // row(ctx, [&]() {
    //     color_ramp(ctx, alia_srgb8{0x8B, 0x43, 0x67});
    //     color_ramp(ctx, alia_srgb8{0xff, 0x64, 0x64});
    // });
}

void
theme_demo(context& ctx)
{
    alia_palette const* p = ctx.palette;
    with_spacing(ctx, 4, [&]() {
        column(ctx, [&]() {
            do_text(ctx, 1, GRAY, 24, "Foundation");
            show_foundation_ramp(ctx, &p->foundation.background);
            show_foundation_ramp(ctx, &p->foundation.structural);
            show_foundation_ramp(ctx, &p->foundation.text);

            do_text(ctx, 1, GRAY, 24, "Semantics");
            show_palette_swatch(ctx, &p->primary);
            show_palette_swatch(ctx, &p->secondary);
            show_palette_swatch(ctx, &p->success);
            show_palette_swatch(ctx, &p->warning);
            show_palette_swatch(ctx, &p->danger);
            show_palette_swatch(ctx, &p->info);

            do_text(ctx, 1, GRAY, 24, "Colors");
            show_palette_swatch(ctx, &p->colors.red);
            show_palette_swatch(ctx, &p->colors.orange);
            show_palette_swatch(ctx, &p->colors.amber);
            show_palette_swatch(ctx, &p->colors.yellow);
            show_palette_swatch(ctx, &p->colors.lime);
            show_palette_swatch(ctx, &p->colors.green);
            show_palette_swatch(ctx, &p->colors.teal);
            show_palette_swatch(ctx, &p->colors.cyan);
            show_palette_swatch(ctx, &p->colors.blue);
            show_palette_swatch(ctx, &p->colors.indigo);
            show_palette_swatch(ctx, &p->colors.purple);
            show_palette_swatch(ctx, &p->colors.pink);
        });
    });
}

void
the_demo(context& ctx)
{
    try
    {
        static int active_demo = 0;
        int const demo_count = 6;
        with_spacing(ctx, 0, [&] {
            row(ctx, [&]() {
                column(ctx, [&]() {
                    for (int i = 0; i < demo_count; ++i)
                    {
                        if (do_rect(
                                ctx,
                                1,
                                {40, 40},
                                (active_demo == i) ? rgba{1, 1, 1, 1}
                                                   : rgba{0, 0, 0, 0},
                                FILL))
                        {
                            active_demo = i;
                            abort_pass();
                        }
                        // do_rect(
                        //     ctx, 1, {1, 1}, rgba{0.4f, 0.4f, 0.4f, 1},
                        //     FILL);
                    }
                });
                column(ctx, GROW, [&]() {
                    inset(
                        ctx,
                        {.left = 40, .right = 40, .top = 40, .bottom = 40},
                        [&]() {
                            with_spacing(ctx, 10, [&] {
                                switch (active_demo)
                                {
                                    case 0:
                                        mixed_flow_demo(ctx);
                                        break;
                                    case 1:
                                        layout_demo(ctx);
                                        break;
                                    case 2:
                                        color_demo(ctx);
                                        break;
                                    case 3:
                                        theme_demo(ctx);
                                        break;
                                    case 4:
                                        grid_demo(ctx);
                                        break;
                                    case 5:
                                        simple_text_demo(ctx);
                                        break;
                                }
                            });
                        });
                });
            });
        });
    }
    catch (pass_aborted)
    {
    }
}

#endif
