#ifndef ALIA_MENU_INTERFACE_HPP
#define ALIA_MENU_INTERFACE_HPP

#include <alia/menu/context.hpp>

namespace alia {

class menu
{
 public:
    menu(menu_context& ctx, std::string const& text, bool enabled = true)
    {
        ctx_ = &ctx;
        ctx.begin_menu(text, enabled);
        active_ = true;
    }
    ~menu() { end(); }
    void end()
    {
        if (active_)
        {
            ctx_->end_menu();
            active_ = false;
        }
    }
 private:
    menu_context* ctx_;
    bool active_;
};

static inline bool do_option(
    menu_context& ctx, std::string const& text, bool enabled = true)
{ return ctx.do_option(text, enabled); }

static inline bool do_checkable_option(
    menu_context& ctx, accessor<bool> const& value,
    std::string const& text, bool enabled = true)
{ return ctx.do_checkable_option(value, text, enabled); }

static inline void do_separator(menu_context& ctx)
{ ctx.do_separator(); }

class menu_controller
{
 public:
    virtual ~menu_controller() {}
    virtual void do_menu(menu_context& ctx) = 0;
};

}

#endif
