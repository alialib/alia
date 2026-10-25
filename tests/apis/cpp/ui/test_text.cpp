#include <alia/ui/text.hpp>

#include <doctest/doctest.h>

using namespace alia;

// Instantiate the public overloads. Calling them requires a text engine.
void
instantiate_text_overloads(ui_context ctx, alia_text_style const* style)
{
    text(ctx, "hello");
    text(ctx, "hello", FLUSH);
    text(ctx, "hello", style);
    text(ctx, "hello", style, FLUSH | width(40.f));
    text(ctx, alia_text_literal("hello"));
    text(ctx, alia_text_with_revision("hello", 5, 1), style, CENTER_Y);
}

TEST_CASE("text component overloads instantiate")
{
    void (*fn)(ui_context, alia_text_style const*)
        = instantiate_text_overloads;
    CHECK(fn != nullptr);
}
