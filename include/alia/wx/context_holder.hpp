#ifndef ALIA_WX_CONTEXT_HOLDER_HPP
#define ALIA_WX_CONTEXT_HOLDER_HPP

#include <alia/forward.hpp>
#include <alia/vector.hpp>
#include <alia/point.hpp>
#include <alia/wx/forward.hpp>
#include <alia/wx/wx.hpp>
#include <alia/context.hpp>
#include <alia/artist.hpp>
#include <alia/wx/opengl_surface.hpp>
//#include <alia/wx/artist.hpp>
//#include <alia/native/artist.hpp>
#include <boost/scoped_ptr.hpp>

namespace alia { namespace wx {

class context_holder
{
 public:
    context_holder(controller* controller);
    ~context_holder();

    alia::context context;
    boost::scoped_ptr<opengl_surface> surface;
    boost::scoped_ptr<alia::artist> artist;

    void set_artist(alia::artist* artist);

    vector2i initial_size;
    vector2i minimum_size;
    vector2i maximum_size;
    bool fit_to_contents;

    void create(wxWindow* parent);

    void adjust_to_content_size(vector2i const& content_size);

    virtual void update();
    virtual void resize(vector2i const& new_size) = 0;
    virtual void close() = 0;

    // If the surface is destroyed, this is called.
    virtual void on_close() {}

    // This is called by the surface to give the parent window a change to
    // process key events. It should true if the key actually did something.
    virtual bool process_key(wxKeyEvent& event) { return false; }

    void adjust_font_sizes();
    void update_color_scheme();
};

}}

#endif
