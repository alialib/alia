#ifndef ALIA_UI_BACKENDS_WIN32_HPP
#define ALIA_UI_BACKENDS_WIN32_HPP

#include <alia/ui/api.hpp>
#include <alia/ui/backends/interface.hpp>

namespace alia {

struct native_window : app_window
{
    native_window() : impl_(0) {}
    native_window(string const& title,
        alia__shared_ptr<app_window_controller> const& controller,
        app_window_state const& initial_state)
    {
        initialize(title, controller, initial_state);
    }
    ~native_window();

    void initialize(string const& title,
        alia__shared_ptr<app_window_controller> const& controller,
        app_window_state const& initial_state);

    alia::ui_system& ui();

    app_window_state state() const;

    bool is_full_screen() const;
    void set_full_screen(bool fs);

    void close();

    bool has_idle_work();
    void do_idle_work();

    void do_message_loop();

    struct impl_data;
 private:
    impl_data* impl_;
};

}

#endif
