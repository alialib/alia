#ifndef ALIA_WX_CALLBACK_HPP
#define ALIA_WX_CALLBACK_HPP

#include <alia/wx/standard_dialogs.hpp>

namespace alia { namespace wx {

// Whenever a callback function is invoked as a reponse to UI input, it
// should be done through invoke_callback().  invoke_callback() wraps the call
// with properly exception handling (not letting exceptions leak out into the
// wxWidgets and/or OS framework) and ensures that the whole GUI system is
// properly updated.
template<class Callback>
void invoke_callback(Callback const& callback)
{
    try
    {
        callback();
        manager::get_instance().update();
    }
    catch(std::exception& e)
    {
        show_error(e.what());
    }
    catch(...)
    {
        show_error("An unknown error has occurred.");
    }
}

// same as above, but doesn't update the whole UI
template<class Callback>
void invoke_callback_without_update(Callback const& callback)
{
    try
    {
        callback();
    }
    catch(std::exception& e)
    {
        show_error(e.what());
    }
    catch(...)
    {
        show_error("An unknown error has occurred.");
    }
}

}}

#endif
