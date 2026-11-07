#include <alia/abi/ui/styling/context_api.h>

#include <alia/impl/base/stack.hpp>

#include <new>

namespace {

template<class T>
struct field_scope
{
    T parent;
};

template<class T>
void
push_style_field(alia_ui_context* ctx, T alia_style_context::* field, T value)
{
    auto& scope = alia::stack_push<field_scope<T>>(alia_ctx_kernel(ctx));
    scope.parent = alia_ctx_style(ctx)->*field;
    alia_ctx_style(ctx)->*field = value;
}

template<class T>
void
pop_style_field(alia_ui_context* ctx, T alia_style_context::* field)
{
    auto& scope = alia::stack_pop<field_scope<T>>(alia_ctx_kernel(ctx));
    alia_ctx_style(ctx)->*field = scope.parent;
}

template<class T>
void
push_focus_field(alia_ui_context* ctx, T alia_focus::* field, T value)
{
    auto& scope = alia::stack_push<field_scope<T>>(alia_ctx_kernel(ctx));
    scope.parent = alia_ctx_style(ctx)->focus.*field;
    alia_ctx_style(ctx)->focus.*field = value;
}

template<class T>
void
pop_focus_field(alia_ui_context* ctx, T alia_focus::* field)
{
    auto& scope = alia::stack_pop<field_scope<T>>(alia_ctx_kernel(ctx));
    alia_ctx_style(ctx)->focus.*field = scope.parent;
}

template<class T>
void
push_text_field(alia_ui_context* ctx, T alia_text_style::* field, T value)
{
    auto& scope = alia::stack_push<field_scope<T>>(alia_ctx_kernel(ctx));
    scope.parent = alia_ctx_style(ctx)->text.*field;
    alia_ctx_style(ctx)->text.*field = value;
}

template<class T>
void
pop_text_field(alia_ui_context* ctx, T alia_text_style::* field)
{
    auto& scope = alia::stack_pop<field_scope<T>>(alia_ctx_kernel(ctx));
    alia_ctx_style(ctx)->text.*field = scope.parent;
}

struct text_style_scope
{
    alia_text_style parent;
};

} // namespace

extern "C" {

void
alia_font_push(alia_ui_context* ctx, alia_resolved_font const* font)
{
    ALIA_ASSERT(ctx && font);
    push_text_field(ctx, &alia_text_style::font, font);
}

void
alia_font_pop(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    pop_text_field(ctx, &alia_text_style::font);
}

void
alia_text_color_push(alia_ui_context* ctx, alia_srgba8 color)
{
    ALIA_ASSERT(ctx);
    push_text_field(ctx, &alia_text_style::color, color);
}

void
alia_text_color_pop(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    pop_text_field(ctx, &alia_text_style::color);
}

void
alia_text_style_push(alia_ui_context* ctx, alia_text_style style)
{
    ALIA_ASSERT(ctx && style.font);
    auto& scope = alia::stack_push<text_style_scope>(alia_ctx_kernel(ctx));
    scope.parent = alia_ctx_style(ctx)->text;
    alia_ctx_style(ctx)->text = style;
}

void
alia_text_style_pop(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    auto& scope = alia::stack_pop<text_style_scope>(alia_ctx_kernel(ctx));
    alia_ctx_style(ctx)->text = scope.parent;
}

void
alia_spacing_push(alia_ui_context* ctx, float spacing)
{
    ALIA_ASSERT(ctx);
    push_style_field(ctx, &alia_style_context::spacing, spacing);
}

void
alia_spacing_pop(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    pop_style_field(ctx, &alia_style_context::spacing);
}

void
alia_focus_color_push(alia_ui_context* ctx, alia_srgba8 color)
{
    ALIA_ASSERT(ctx);
    push_focus_field(ctx, &alia_focus::color, color);
}

void
alia_focus_color_pop(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    pop_focus_field(ctx, &alia_focus::color);
}

void
alia_focus_show_on_pointer_push(alia_ui_context* ctx, bool show)
{
    ALIA_ASSERT(ctx);
    push_focus_field(ctx, &alia_focus::show_on_pointer, show);
}

void
alia_focus_show_on_pointer_pop(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    pop_focus_field(ctx, &alia_focus::show_on_pointer);
}

void
alia_focus_ring_push(alia_ui_context* ctx, alia_focus_ring ring)
{
    ALIA_ASSERT(ctx);
    push_focus_field(ctx, &alia_focus::ring, ring);
}

void
alia_focus_ring_pop(alia_ui_context* ctx)
{
    ALIA_ASSERT(ctx);
    pop_focus_field(ctx, &alia_focus::ring);
}

} // extern "C"
