#include "demo.hpp"

namespace custom_context {

// clang-format off
/// [custom-context]

// Define our username context tag.
// At the moment, all context objects must be stored by reference, so although
// we wouldn't normally use a reference for a readable<std::string>, we have to
// do so here.
ALIA_DEFINE_TAGGED_TYPE(username_tag, readable<std::string>&)

// Define our app's context type by extending the asm-dom context type.
typedef extend_context_type_t<html::context, username_tag> app_context;

// The functions that define the internal portions of our app UI use
// app_context and have access to the username via that context...
void
internal_app_ui(app_context ctx)
{
    html::text(ctx,
        alia::printf(ctx, "Welcome, %s!", get<username_tag>(ctx)));
}

// Our top-level UI function takes the context that the asm-dom wrapper provides
// and extends it to what our app needs...
void
main_app_ui(html::context ctx)
{
    // Get the username.
    // (Maybe in a real app this wouldn't be hardcoded...)
    readable<std::string> username = value("tmadden");

    // Extend the app context to include the username.
    app_context app_ctx = extend_context<username_tag>(ctx, username);

    // Pass that context along to the internal portions of the app UI...
    internal_app_ui(app_ctx);
}
/// [custom-context]
// clang-format on

void
init_demo(std::string dom_id)
{
    static alia::system the_system;
    static html::system the_dom;

    initialize(the_dom, the_system, dom_id, [](html::context ctx) {
        main_app_ui(ctx);
    });
}

static demo the_demo("custom-context", init_demo);

} // namespace custom_context
