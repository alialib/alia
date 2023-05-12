#include "demo.hpp"

namespace custom_context {

// clang-format off
/// [custom-context]

// Define our username context tag.
// The object that we associate with it will be a read-only string signal.
ALIA_DEFINE_TAGGED_TYPE(username_tag, readable<std::string>)

// Define our app's context type by extending the alia/HTML context type.
typedef extend_context_type_t<html::context, username_tag> app_context;

// The functions that define the internal portions of our app UI use
// app_context and have access to the username via that context...
void
internal_app_ui(app_context ctx)
{
    html::p(ctx,
        alia::printf(ctx, "Welcome, %s!", get<username_tag>(ctx)));
}

// Our top-level UI function takes the context that alia/HTML provides and
// extends it to what our app needs...
void
main_app_ui(html::context ctx)
{
    // Get the username.
    // (Maybe in a real app this wouldn't be hardcoded...)
    auto username = value("tmadden");

    // Extend the context by associating our username tag with its value.
    with_extended_context<username_tag>(ctx, username,
        // The 'ctx' parameter that we get inside this lambda is our extended
        // context.
        [&](auto ctx) {
            // Pass that context along to the internal portions of the app UI.
            internal_app_ui(ctx);
        });
}
/// [custom-context]
// clang-format on

void
init_demo(std::string dom_id)
{
    static html::system the_system;

    initialize(
        the_system, dom_id, [](html::context ctx) { main_app_ui(ctx); });
}

static demo the_demo("custom-context", init_demo);

} // namespace custom_context
