#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

// This file defines our application's model data and functions.
//
// Note that there's nothing related to alia in here. This is all just normal
// C++ code.
//
// Normally, you would probably split this up into .hpp and .cpp files, but for
// easier digesting, the entire model is in this file...
//

// DATA TYPES - Here we define our application data model in terms of C++ data
// structures...

struct todo_item
{
    bool completed;
    std::string title;
    int id;

    // The following disables copying of this struct (but keeps the default
    // move constructor and move assignment operator).
    //
    // You wouldn't normally do this. We do it here just to demonstrate that
    // our app's dataflow uses purely move semantics and isn't actually copying
    // around whole vectors of items (or even individual ones).
    //
    todo_item(todo_item const&) = delete;
    todo_item&
    operator=(todo_item const&)
        = delete;
    todo_item(todo_item&&) = default;
    todo_item&
    operator=(todo_item&&)
        = default;
    todo_item() = default;
};

typedef std::vector<todo_item> todo_list;

struct app_state
{
    todo_list todos;
    int next_id = 0;
};

enum class item_filter
{
    ALL,
    ACTIVE,
    COMPLETED
};

// FUNCTIONS - The following functions define the queries and transformations
// that our app needs to perform on our data model. Note that the interfaces
// follow a functional, by-value style, but the internals freely use mutation
// to achieve their goals.

// Add a new TODO item to our app's list of items.
app_state
add_todo(app_state state, std::string title)
{
    state.todos.push_back(todo_item{false, title, state.next_id});
    ++state.next_id;
    return state;
}

// Get the number of incomplete items in a TODO list.
size_t
incomplete_count(todo_list const& todos)
{
    return std::count_if(todos.begin(), todos.end(), [](auto const& item) {
        return !item.completed;
    });
}

// Clear all completed items from a TODO list.
todo_list
clear_completed(todo_list todos)
{
    todos.erase(
        std::remove_if(
            todos.begin(),
            todos.end(),
            [](todo_item const& item) { return item.completed; }),
        todos.end());
    return todos;
}

// Set the 'completed' flag on all items in a TODO list to a new value.
todo_list
set_completed_flags(todo_list todos, bool new_value)
{
    for (auto& todo : todos)
        todo.completed = new_value;
    return todos;
}

// Does the given TODO item match the given filter?
bool
matches_filter(item_filter filter, todo_item const& item)
{
    return item.completed && filter != item_filter::ACTIVE
           || !item.completed && filter != item_filter::COMPLETED;
}

// Trim a string.
// (Somehow we still don't have this in the C++ standard library!?!)
std::string
trim(std::string const& str)
{
    auto const begin = str.find_first_not_of(" ");
    if (begin == std::string::npos)
        return "";

    auto const end = str.find_last_not_of(" ");

    return str.substr(begin, end - begin + 1);
}

// CEREAL INTERFACES - This demo uses Cereal to convert our app state to and
// from JSON for external storage in the browser.

#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>

template<class Archive>
void
serialize(Archive& archive, todo_item& item)
{
    archive(item.completed, item.title, item.id);
}

template<class Archive>
void
serialize(Archive& archive, app_state& state)
{
    archive(state.todos, state.next_id);
}

std::string
app_state_to_json(app_state const& state)
{
    std::ostringstream stream;
    {
        cereal::JSONOutputArchive archive(stream);
        archive(state);
    }
    return stream.str();
}

app_state
json_to_app_state(std::string json)
{
    try
    {
        std::istringstream stream(std::move(json));
        app_state state;
        {
            cereal::JSONInputArchive archive(stream);
            archive(state);
        }
        return state;
    }
    catch (...)
    {
        // If anything goes wrong, just return a fresh state.
        return app_state();
    }
}
