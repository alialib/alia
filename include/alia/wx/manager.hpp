#ifndef ALIA_WX_MANAGER_HPP
#define ALIA_WX_MANAGER_HPP

#include <alia/wx/forward.hpp>
#include <alia/singleton.hpp>

namespace alia { namespace wx {

class manager : public singleton<manager>
{
 public:
    void register_(impl::top_level_window* window);
    void unregister(impl::top_level_window* window);

    void push_modal(impl::top_level_window* window);
    void pop_modal(impl::top_level_window* window);

    unsigned get_window_count() const;

    void update();

    void exit();
    bool exiting() const;

    void set_font_size_adjustment(float adjustment);
    float get_font_size_adjustment() const;

    void set_color_scheme(unsigned scheme);
    unsigned get_color_scheme() const;

 private:
    manager();
    ~manager();
    friend class singleton<manager>;

    void close_all();

    struct impl_data;
    impl_data* impl_;
};

}}

#endif
