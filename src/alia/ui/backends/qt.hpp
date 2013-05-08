#ifndef ALIA_UI_BACKENDS_QT_HPP
#define ALIA_UI_BACKENDS_QT_HPP

#include <alia/ui/api.hpp>
#include <alia/ui/backends/interface.hpp>

namespace alia {

struct qt_window : app_window
{
    qt_window() : impl_(0) {}
    qt_window(string const& title,
        alia__shared_ptr<app_window_controller> const& controller,
        app_window_state const& initial_state)
    {
        initialize(title, controller, initial_state);
    }
    ~qt_window();

    void initialize(string const& title,
        alia__shared_ptr<app_window_controller> const& controller,
        app_window_state const& initial_state);

    alia::ui_system& ui();

    app_window_state state() const;

    bool is_full_screen() const;
    void set_full_screen(bool fs);

    struct impl_data;
 private:
    impl_data* impl_;
};

}

#endif
