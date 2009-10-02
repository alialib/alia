#include <alia/wx/manager.hpp>
#include <alia/wx/impl.hpp>
#include <list>
#include <cassert>

namespace alia { namespace wx {

struct manager::impl_data
{
    std::list<impl::top_level_window*> modals, windows;
    bool exit_requested, already_closed;
    float font_size_adjustment;
    unsigned color_scheme;
};

manager::manager()
{
    impl_ = new impl_data;
    impl_->exit_requested = false;
    impl_->already_closed = false;
    impl_->font_size_adjustment = 0;
    impl_->color_scheme = 0;
}
manager::~manager()
{
    delete impl_;
}

void manager::push_modal(impl::top_level_window* window)
{
    impl_->modals.push_front(window);
}

void manager::pop_modal(impl::top_level_window* window)
{
    assert(!impl_->modals.empty() && impl_->modals.front() == window);
    impl_->modals.pop_front();
}

void manager::update()
{
    if (impl_->exit_requested)
    {
        if (!impl_->already_closed)
        {
            // This has to be set before calling close_all() in case of
            // recursive update() calls while closing.
            impl_->already_closed = true;
            close_all();
        }
        return;
    }

    if (impl_->modals.empty())
    {
        for (std::list<impl::top_level_window*>::const_iterator
            i = impl_->windows.begin(); i != impl_->windows.end(); ++i)
        {
            (*i)->update();
        }
    }
    else
    {
        impl::top_level_window* active = impl_->modals.front();
        active->update();
    }
}

void manager::close_all()
{
    for (std::list<impl::top_level_window*>::const_iterator
        i = impl_->windows.begin(); i != impl_->windows.end(); ++i)
    {
        (*i)->close();
    }
}

void manager::register_(impl::top_level_window* window)
{
    impl_->windows.push_back(window);
}

void manager::unregister(impl::top_level_window* window)
{
    impl_->windows.remove(window);
}

unsigned manager::get_window_count() const
{
    return int(impl_->windows.size());
}

void manager::exit()
{
    impl_->exit_requested = true;
}

bool manager::exiting() const
{
    return impl_->exit_requested;
}

void manager::set_font_size_adjustment(float adjustment)
{
    impl_->font_size_adjustment = adjustment;
    for (std::list<impl::top_level_window*>::const_iterator
        i = impl_->windows.begin(); i != impl_->windows.end(); ++i)
    {
        (*i)->adjust_font_sizes();
    }
}
float manager::get_font_size_adjustment() const
{
    return impl_->font_size_adjustment;
}

void manager::set_color_scheme(unsigned scheme)
{
    impl_->color_scheme = scheme;
    for (std::list<impl::top_level_window*>::const_iterator
        i = impl_->windows.begin(); i != impl_->windows.end(); ++i)
    {
        (*i)->update_color_scheme();
    }
}
unsigned manager::get_color_scheme() const
{
    return impl_->color_scheme;
}

}}
