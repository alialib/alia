#ifndef ALIA_UI_LIBRARY_BULLETS_HPP
#define ALIA_UI_LIBRARY_BULLETS_HPP

#include "alia/core/context/interface.hpp"
#include <alia/ui/common.hpp>
#include <alia/ui/context.hpp>
#include <alia/ui/layout/grids.hpp>
#include <alia/ui/layout/specification.hpp>

namespace alia {

void
do_bullet(ui_context ctx, layout const& layout_spec = default_layout);

struct bulleted_list : noncopyable
{
    bulleted_list()
    {
    }
    bulleted_list(ui_context ctx, layout const& layout_spec = default_layout)
    {
        begin(ctx, layout_spec);
    }
    ~bulleted_list()
    {
        end();
    }

    void
    begin(ui_context ctx, layout const& layout_spec = default_layout);

    void
    end();

 private:
    friend struct bulleted_item;
    optional_context<ui_context> ctx_;
    grid_layout grid_;
};

struct bulleted_item : noncopyable
{
    bulleted_item()
    {
    }
    bulleted_item(
        bulleted_list& list, layout const& layout_spec = default_layout)
    {
        begin(list, layout_spec);
    }
    ~bulleted_item()
    {
        end();
    }

    void
    begin(bulleted_list& list, layout const& layout_spec = default_layout);

    void
    end();

 private:
    grid_row row_;
};

} // namespace alia

#endif
