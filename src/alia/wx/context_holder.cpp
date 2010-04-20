#include <alia/wx/context_holder.hpp>
#include <alia/wx/artist.hpp>
#include <alia/native/artist.hpp>
#include <alia/wx/manager.hpp>
#include <alia/controller.hpp>
#include <alia/layout.hpp>
#include <alia/skia/artist.hpp>

namespace alia { namespace wx {

context_holder::context_holder(controller* controller)
{
    initial_size = vector2i(-1, -1);
    minimum_size = vector2i(-1, -1);
    maximum_size = vector2i(-1, -1);
    fit_to_contents = false;

    context.controller = controller;

    surface.reset(new opengl_surface);
    surface->set_holder(this);
    context.surface = surface.get();

    context.font_scale_factor =
        manager::get_instance().get_font_scale_factor();

 //#ifdef ALIA_HAS_NATIVE_ARTIST
 //   try
 //   {
 //       artist.reset(new native::artist);
 //   }
 //   catch (...)
 //   {
 //       artist.reset(new wx::artist);
 //   }
 //#else
 //   artist.reset(new wx::artist);
 //#endif

    set_artist(new skia::artist);
}

void context_holder::set_artist(alia::artist* artist)
{
    this->artist.reset(artist);
    artist->set_context(context);
    context.artist = artist;
    artist->initialize();
    artist->set_color_scheme(manager::get_instance().get_color_scheme());
}

context_holder::~context_holder()
{
    delete context.controller;
}

void context_holder::create(wxWindow* parent)
{
    surface->create(parent);

    // Do an initial layout pass to get the minimum size that will fit the
    // contents.  If no initial size was specified for the surface, this is
    // used.
    do_layout(context);
    vector2i content_size = context.content_size;

    // Determine the initial size of the surface.
    vector2i size;
    for (int i = 0; i < 2; ++i)
    {
        size[i] = initial_size[i] < 0 ? content_size[i] : initial_size[i];
        if (size[i] < minimum_size[i])
            size[i] = minimum_size[i];
        if (maximum_size[i] >= 0 && size[i] > maximum_size[i])
            size[i] = maximum_size[i];
    }
    initial_size = size;

    surface->get_wx_window()->SetMinSize(wxSize(
        (std::max)(minimum_size[0], 0),
        (std::max)(minimum_size[1], 0)));

    // Create a sizer, and make sure the surface fills it.
    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(surface->get_wx_window(), 1, wxEXPAND, 0);

    // Tie the sizer and the parent window together.
    parent->SetSizer(sizer);
    sizer->SetSizeHints(parent);

    // Now set the actual size.
    parent->SetClientSize(size[0], size[1]);

    do_layout(context);
    {
    init_event e;
    issue_event(context, e);
    }

    update();
}

void context_holder::update()
{
    surface->update();
}

void context_holder::adjust_to_content_size(vector2i const& content_size)
{
    if (fit_to_contents)
    {
        vector2i size;
        for (int i = 0; i < 2; ++i)
        {
            size[i] = content_size[i] < 0 ? content_size[i] : content_size[i];
            if (size[i] < minimum_size[i])
                size[i] = minimum_size[i];
            if (maximum_size[i] >= 0 &&
                size[i] > maximum_size[i])
            {
                size[i] = maximum_size[i];
            }
        }
        resize(size);
    }
}

void context_holder::adjust_font_sizes()
{
    set_font_scale_factor(context,
        manager::get_instance().get_font_scale_factor());
}

void context_holder::update_color_scheme()
{
    artist->set_color_scheme(manager::get_instance().get_color_scheme());
}

}}
