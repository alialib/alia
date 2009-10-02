#ifndef ALIA_WX_WINDOW_HPP
#define ALIA_WX_WINDOW_HPP

#include <alia/forward.hpp>
#include <alia/wx/forward.hpp>
#include <alia/wx/top_level_window.hpp>
#include <alia/menu/context.hpp>
#include <alia/controller.hpp>
#include <string>

namespace alia { namespace wx {

class window_controller : public controller
{
 public:
    virtual void do_menu_bar(menu_context& ctx) {}
    virtual void on_close() {}
    wx::window* window;
};

class window : public top_level_window
{
 public:
    window(std::string const& title, window_controller* controller);
    ~window();

    point2i get_position() const;
    void set_position(point2i const& position);

    vector2i get_size() const;
    void set_size(vector2i const& new_size);

    bool is_maximized() const;
    void set_maximized(bool maximized);

    bool is_fit_to_contents() const;
    void set_fit_to_contents(bool fit_to_contents);

    bool is_resizable() const;
    void set_resizable(bool resizable);

    vector2i const& get_minimum_size() const;
    void set_minimum_size(vector2i const& minimum_size);

    vector2i const& get_maximum_size() const;
    void set_maximum_size(vector2i const& maximum_size);

    bool is_full_screen() const;
    void set_full_screen(bool fs);

    void create();

    bool is_open() const;
    void close();

 private:
    void* get_wx_window() const;

    friend class impl::frame;
    impl::frame* impl_;
};

}}

#endif
