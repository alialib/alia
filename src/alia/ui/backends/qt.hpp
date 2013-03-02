#ifndef ALIA_UI_BACKENDS_QT_HPP
#define ALIA_UI_BACKENDS_QT_HPP

#include <alia/ui/api.hpp>

// TODO: refactor wrt win32.hpp

namespace alia {

struct qt_window;

struct window_controller : ui_controller
{
    qt_window* window;
};

struct qt_error : exception
{
    qt_error(string const& msg)
      : exception(msg)
    {}
    ~qt_error() throw() {}
};

struct window_state_flag_tag {};
typedef flag_set<window_state_flag_tag> window_state_flag_set;

ALIA_DEFINE_FLAG(window_state, 0x1, MAXIMIZED)
ALIA_DEFINE_FLAG(window_state, 0x2, FULL_SCREEN)

struct ui_system;

struct qt_window : noncopyable
{
    struct state_data
    {
        optional<vector<2,int> > position;
        // size is the size of the window when it's in it's normal state
        // (i.e., not maximized or full screen).
        vector<2,int> size;
        window_state_flag_set flags;

        state_data() {}
        state_data(
            optional<vector<2,int> > const& position,
            vector<2,int> const& size,
            window_state_flag_set flags = NO_FLAGS)
          : position(position), size(size), flags(flags)
        {}
    };

    qt_window() : impl_(0) {}
    qt_window(string const& title, window_controller* controller,
        state_data const& initial_state)
    {
        initialize(title, controller, initial_state);
    }
    ~qt_window();

    void initialize(string const& title, window_controller* controller,
        state_data const& initial_state);

    alia::ui_system& ui();

    state_data state() const;

    bool is_full_screen() const;
    void set_full_screen(bool fs);

    struct impl_data;
 private:
    impl_data* impl_;
};

}

#endif
