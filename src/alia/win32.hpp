#ifndef ALIA_WIN32_HPP
#define ALIA_WIN32_HPP

#include <alia/ui_interface.hpp>

// TODO: remove
#include <boost/optional.hpp>

namespace alia {

struct native_window;

struct window_controller : ui_controller
{
    native_window* window;
};

struct native_error : exception
{
    native_error(string const& msg)
      : exception(msg)
    {}
    ~native_error() throw() {}
};

struct window_state_flag_tag {};
typedef flag_set<window_state_flag_tag> window_state_flag_set;

ALIA_DEFINE_FLAG_CODE(window_state_flag_tag, 0x1, MAXIMIZED)
ALIA_DEFINE_FLAG_CODE(window_state_flag_tag, 0x2, FULL_SCREEN)

struct ui_system;

struct native_window : noncopyable
{
    struct state_data
    {
        boost::optional<vector<2,int> > position;
        // size is the size of the window when it's in it's normal state
        // (i.e., not maximized or full screen).
        vector<2,int> size;
        window_state_flag_set flags;

        state_data() {}
        state_data(
            boost::optional<vector<2,int> > const& position,
            vector<2,int> const& size,
            window_state_flag_set flags = NO_FLAGS)
          : position(position), size(size), flags(flags)
        {}
    };

    native_window(string const& title, window_controller* controller,
        state_data const& initial_state);
    ~native_window();

    alia::ui_system& ui();

    state_data state() const;

    bool is_full_screen() const;
    void set_full_screen(bool fs);

    bool has_idle_work();
    void do_idle_work();

    struct impl_data;
 private:
    impl_data* impl_;
};

}

#endif
