#ifndef ALIA_UI_UTILITIES_STYLING_HPP
#define ALIA_UI_UTILITIES_STYLING_HPP

#include <alia/ui/context.hpp>
#include <alia/ui/styling.hpp>

namespace alia {

class scoped_style_info : noncopyable
{
 public:
    scoped_style_info()
    {
    }
    template<class Context>
    scoped_style_info(Context& ctx)
    {
        begin(ctx);
    }
    template<class Context>
    scoped_style_info(Context& ctx, style_info const& style)
    {
        begin(ctx);
        set(style);
    }
    ~scoped_style_info()
    {
        end();
    }
    void
    begin(ui_context ctx)
    {
        ctx_.reset(ctx);
        old_style_ = get_style(ctx);
    }
    void
    set(style_info const& style)
    {
        get_style(*ctx_) = style;
    }
    void
    end()
    {
        get_style(*ctx_) = old_style_;
    }

 private:
    optional_context<ui_context> ctx_;
    style_info old_style_;
};

} // namespace alia

#endif
