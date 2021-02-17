#include <alia/html.hpp>
#include <alia/html/routing.hpp>

#include "model.hpp"

using namespace alia;

// Define the objects in our app's context.
ALIA_DEFINE_TAGGED_TYPE(app_state_tag, duplex<app_state>)
ALIA_DEFINE_TAGGED_TYPE(view_filter_tag, readable<item_filter>)
typedef extend_context_type_t<html::context, app_state_tag, view_filter_tag>
    app_context;

// Do the UI for adding a new TODO item to the list.
void
new_todo_ui(app_context ctx)
{
    // Get some component-local state to store the title of the new TODO.
    auto new_todo = get_state(ctx, std::string());
    // And present an input for editing it.
    input(ctx, new_todo)
        .class_("new-todo")
        .placeholder("What needs to be done?")
        .autofocus()
        .on_enter(
            // In response to the enter key, we need to:
            // - Trim the new_todo string.
            // - Check that it's not empty. (Abort if it is.)
            // - Invoke 'add_todo' to add it to our app's state.
            // - Reset new_todo to an empty string.
            (actions::apply(
                 add_todo,
                 get<app_state_tag>(ctx),
                 hide_if_empty(lazy_apply(trim, new_todo))),
             new_todo <<= ""));
}

// Do the UI for a single TODO item.
void
todo_item_ui(app_context ctx, size_t index, duplex<todo_item> todo)
{
    // The UI for a single item has two modes: editing and viewing, so we need
    // a bit of state to track which we're in.
    auto editing = get_transient_state(ctx, false);

    li(ctx)
        .class_("completed", alia_field(todo, completed))
        .class_("editing", editing)
        .content([&] {
            alia_if(editing)
            {
                // In editing mode, we need to track the new value for the
                // title of the item. (The edit is cancellable, so we don't
                // want to write it back until the edit is confirmed.)
                auto new_title
                    = get_transient_state(ctx, alia_field(todo, title));

                // Define the action that we'll perform to save the edit and
                // transition out of editing mode.
                auto save
                    = (alia_field(todo, title) <<= new_title,
                       editing <<= false);

                input(ctx, new_title)
                    .class_("edit")
                    .init([](auto& self) { focus(self); })
                    // If the edit control loses focus or Enter is pressed,
                    // save the edits.
                    .on("blur", save)
                    .on_enter(save)
                    // Escape discards the new title and exits editing mode.
                    .on_escape(editing <<= false);
            }
            alia_else
            {
                // In viewing mode, we need a "view" div to contain our viewing
                // elements.
                auto view = div(ctx, "view");
                view.content([&] {
                    // This checkbox is hooked up to the 'completed' field of
                    // the TODO item.
                    checkbox(ctx, alia_field(todo, completed))
                        .class_("toggle");

                    label(ctx, alia_field(todo, title))
                        // Double clicking on the title enters editing mode.
                        .on("dblclick", editing <<= true);

                    // If the mouse is hovered over this item view, show a
                    // button that will erase it from the list.
                    alia_if(mouse_inside(ctx, view))
                    {
                        button(
                            ctx,
                            actions::erase_index(
                                alia_field(get<app_state_tag>(ctx), todos),
                                index))
                            .class_("destroy");
                    }
                    alia_end
                });
            }
            alia_end
        });
}

// Tell alia that our TODO items can be stably identified by their 'id' member.
auto
get_alia_id(todo_item const& item)
{
    return make_id(item.id);
}

// Do the UI for the list of TODO items.
void
todo_list_ui(app_context ctx)
{
    auto todos = alia_field(get<app_state_tag>(ctx), todos);
    ul(ctx, "todo-list", [&] {
        for_each(ctx, todos, [&](size_t index, auto todo) {
            auto matches
                = apply(ctx, matches_filter, get<view_filter_tag>(ctx), todo);
            alia_if(matches)
            {
                todo_item_ui(ctx, index, todo);
            }
            alia_end
        });
    });
}

// Do the checkbox that toggles the state of all TODO items.
void
toggle_all_checkbox(app_context ctx, readable<bool> all_complete)
{
    element(ctx, "input")
        .attr("id", "toggle-all")
        .attr("class", "toggle-all")
        .attr("type", "checkbox")
        .prop("checked", !all_complete)
        .on("change",
            actions::apply(
                set_completed_flags,
                alia_field(get<app_state_tag>(ctx), todos),
                !all_complete));
    label(ctx, "Mark all as complete").attr("for", "toggle-all");
}

// Do the UI for selecting an item filter.
void
filter_selection_ui(app_context ctx)
{
    auto filter = get<view_filter_tag>(ctx);
    ul(ctx, "filters", [&] {
        // In the interests of not repeating ourselves, we define a lambda here
        // that generates the UI for a single filter link.
        // (We could also split this out into a separate function.)
        auto filter_link = [&](auto label, auto href, auto value) {
            li(ctx).content([&] {
                link(ctx, label, href).class_("selected", filter == value);
            });
        };
        // Invoke filter_link for each of our filters.
        filter_link("All", "#/", item_filter::ALL);
        filter_link("Active", "#/active", item_filter::ACTIVE);
        filter_link("Completed", "#/completed", item_filter::COMPLETED);
    });
}

// Do the top-level UI for our app content.
void
app_ui(app_context ctx)
{
    auto todos = alia_field(get<app_state_tag>(ctx), todos);

    header(ctx, "header", [&] {
        h1(ctx, "todos");
        new_todo_ui(ctx);
    });

    // None of the following should be shown when the TODO list is empty.
    alia_if(!is_empty(todos))
    {
        auto items_left = apply(ctx, incomplete_count, todos);

        section(ctx, "main", [&] {
            toggle_all_checkbox(ctx, items_left == size(todos));
            todo_list_ui(ctx);
        });

        footer(ctx, "footer", [&] {
            // Display the count of incomplete items.
            span(ctx, "todo-count", [&] {
                strong(ctx, as_text(ctx, items_left));
                text(
                    ctx,
                    conditional(items_left != 1, " items left", " item left"));
            });

            filter_selection_ui(ctx);

            // Add a button to clear all completed items.
            button(
                ctx, "Clear completed", actions::apply(clear_completed, todos))
                .class_("clear-completed");
        });
    }
    alia_end
}

// Translate a browser location hash string to our item_filter enum type.
item_filter
hash_to_filter(std::string const& hash)
{
    if (hash == "#/active")
        return item_filter::ACTIVE;
    if (hash == "#/completed")
        return item_filter::COMPLETED;
    return item_filter::ALL;
}

// root_ui() provides the machinery to tie the app UI into the browser.
void
root_ui(html::context ctx)
{
    // Construct the signal for our applications state...
    // First, create a binding to the raw, JSON state in local storage.
    auto json_state = get_local_state(ctx, "todos-alia");
    // Pass that through a two-way serializer/deserializer to convert it to our
    // native C++ representation.
    auto native_state
        = duplex_apply(ctx, json_to_app_state, app_state_to_json, json_state);
    // And finally, add a default value (of default-initialized state) for when
    // the raw JSON doesn't exist yet.
    auto state = add_default(native_state, default_initialized<app_state>());

    // Parse the location hash to determine the active filter.
    auto filter = apply(ctx, hash_to_filter, get_location_hash(ctx));

    // Add both signals to the alia/HTML context to create our app context.
    with_extended_context<app_state_tag>(ctx, state, [&](auto ctx) {
        with_extended_context<view_filter_tag>(ctx, filter, [&](auto ctx) {
            // Root the app UI in the HTML DOM tree.
            // Our app's UI will be placed at the placeholder HTML element
            // with the ID "app-content". (See index.html.)
            placeholder_root(ctx, "app-content", [&] { app_ui(ctx); });
        });
    });
}

// Our main() just initializes our UI and lets it do its thing.
int
main()
{
    static html::system sys;
    initialize(sys, root_ui);
    enable_hash_monitoring(sys);
};
