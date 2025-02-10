#ifndef ALIA_UI_BACKENDS_INTERFACE_HPP
#define ALIA_UI_BACKENDS_INTERFACE_HPP

#include <optional>

#include <alia/ui/common.hpp>
#include <alia/ui/geometry.hpp>

// This file defines a common interface between the application window and the
// UI controller. Where applicable, it can be implemented by backends.

namespace alia {

struct ui_system;

ALIA_DEFINE_FLAG_TYPE(app_window_state)
ALIA_DEFINE_FLAG(app_window_state, 0x1, APP_WINDOW_MAXIMIZED)
ALIA_DEFINE_FLAG(app_window_state, 0x2, APP_WINDOW_FULL_SCREEN)

struct app_window_state
{
    std::optional<vector<2, int>> position;
    // `size` is the size of the window when it's in it's normal state
    // (i.e., not maximized or full screen).
    vector<2, int> size;
    app_window_state_flag_set flags = NO_FLAGS;
};

struct app_window
{
    // Get the current state of the window.
    virtual app_window_state
    state() const
        = 0;

    // Note that no function is provided for setting the entire window state.
    // It's assumed that the window's initialization function takes an
    // initial state argument (and that that's sufficient).

    // Switch the window between full screen and windowed mode.
    virtual void
    set_full_screen(bool fs)
        = 0;

    // Is the window currently in full screen mode?
    virtual bool
    is_full_screen() const
    {
        return (this->state().flags & APP_WINDOW_FULL_SCREEN) ? true : false;
    }

    // Close this window.
    virtual void
    close()
        = 0;
};

// struct app_window_controller : ui_controller
// {
//     app_window* window;
// };

struct backend_error : exception
{
    backend_error(std::string const& msg) : exception(msg)
    {
    }
    ~backend_error() throw()
    {
    }
};

} // namespace alia

#endif
