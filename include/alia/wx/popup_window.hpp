#ifndef ALIA_WX_POPUP_WINDOW_HPP
#define ALIA_WX_POPUP_WINDOW_HPP

#include <alia/wx/context_holder.hpp>
#include <alia/surface.hpp>

namespace alia { namespace wx {

class popup_window
  : public popup_interface
  , public context_holder
{
 public:
    popup_window(opengl_surface* parent, controller* controller,
        point2i const& initial_position, point2i const& boundary,
        vector2i const& minimum_size, vector2i const& maximum_size);
    ~popup_window();

    void update();
    void orphan();

    // required by context_holder and/or popup_interface...
    void resize(vector2i const& new_size);
    void close();
    bool is_open() const;
    bool was_dismissed() const;

 private:
    void handle_resize();
    void handle_dismiss();
    void handle_focus_loss();

    void open();

    class wx_popup_wrapper;

    struct impl_data;
    impl_data* impl_;
};

}}

#endif
