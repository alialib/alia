#include "demo.hpp"

namespace custom_components {

/// [custom-components]

// Define our username "component".
// At the moment, all context components must be references, so although we
// wouldn't normally use a reference for a readable<std::string>, we have to
// do so here.
ALIA_DEFINE_COMPONENT_TYPE(username_tag, readable<std::string>&)

// Define our app's context type by extending the asm-dom context type.
typedef extend_context_type_t<dom::context, username_tag> app_context;

// The functions that define the internal portions of our app UI use
// app_context and have access to the username via that context...
void
do_internal_app_ui(app_context ctx)
{
    do_text(ctx, printf(ctx, "Welcome, %s!", get_component<username_tag>(ctx)));
}

// Our top-level UI function takes the context that the asm-dom wrapper provides
// and extends it to what our app needs...
void
do_main_app_ui(dom::context ctx)
{
    // Get the username.
    // (Maybe in a real app this wouldn't be hardcoded...)
    readable<std::string> username = value("tmadden");

    // Extend the app context to include the username.
    app_context app_ctx
        = extend_context<username_tag>(copy_context(ctx), username);

    // Pass that context along to the internal portions of the app UI...
    do_internal_app_ui(app_ctx);
}

/// [custom-components]

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_main_app_ui(ctx);
    });
}

static demo the_demo("custom-components", init_demo);

} // namespace custom_components

namespace multiple_custom_components {

// clang-format off
/// [multiple-custom-components]

ALIA_DEFINE_COMPONENT_TYPE(username_tag, readable<std::string>&)
ALIA_DEFINE_COMPONENT_TYPE(api_key_tag, readable<std::string>&)

typedef extend_context_type_t<dom::context, username_tag, api_key_tag>
    app_context;

void
do_internal_app_ui(app_context ctx)
{
    do_text(ctx, printf(ctx, "Welcome, %s!", get_component<username_tag>(ctx)));
    do_text(ctx,
        printf(ctx,
            "Your secret key is %s! Keep it safe!",
            get_component<api_key_tag>(ctx)));
}

void
do_main_app_ui(dom::context ctx)
{
    readable<std::string> username = value("tmadden");
    readable<std::string> api_key = value("abc123");

    app_context app_ctx =
        extend_context<api_key_tag>(
            extend_context<username_tag>(copy_context(ctx), username),
            api_key);

    do_internal_app_ui(app_ctx);
}

/// [multiple-custom-components]
// clang-format on

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static dom::system the_dom;

    initialize(the_dom, the_system, dom_id, [](dom::context ctx) {
        do_main_app_ui(ctx);
    });
}

static demo the_demo("multiple-custom-components", init_demo);

} // namespace multiple_custom_components
