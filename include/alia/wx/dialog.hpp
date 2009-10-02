#ifndef ALIA_WX_DIALOG_HPP
#define ALIA_WX_DIALOG_HPP

#include <alia/forward.hpp>
#include <alia/wx/forward.hpp>
#include <alia/wx/top_level_window.hpp>
#include <alia/controller.hpp>
#include <string>

namespace alia { namespace wx {

class dialog : public top_level_window
{
 public:
    dialog(std::string const& title, controller* controller,
        top_level_window* parent = 0);
    ~dialog();

    point2i get_position() const;
    void set_posiiton(point2i const& position);

    vector2i get_size() const;
    void set_size(vector2i const& new_size);

    bool is_fit_to_contents() const;
    void set_fit_to_contents(bool fit_to_contents);

    bool is_resizable() const;
    void set_resizable(bool resizable);

    vector2i const& get_minimum_size() const;
    void set_minimum_size(vector2i const& minimum_size);

    vector2i const& get_maximum_size() const;
    void set_maximum_size(vector2i const& maximum_size);

    void create();
    bool is_open() const;
    void close();

    void show_modal();

 private:
    void* get_wx_window() const;

    friend class impl::dialog;
    impl::dialog* impl_;
};

}}

#endif
