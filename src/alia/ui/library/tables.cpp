#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

void table::begin(ui_context& ctx, accessor<string> const& style,
    layout const& layout_spec)
{
    ctx_ = &ctx;
    panel_.begin(ctx, style, layout_spec, PANEL_NO_INTERNAL_PADDING);
    cell_style_.begin(ctx, text("cell"));
    grid_.begin(ctx);
    cell_index_ = make_vector<int>(0, 0);
}
void table::end()
{
    if (ctx_)
    {
        grid_.end();
        cell_style_.end();
        ctx_ = 0;
    }
}

void table_row::begin(table& table, layout const& layout_spec)
{
    table_ = &table;
    ui_context& ctx = *table.ctx_;

    ++table.cell_index_[1];
    table.cell_index_[0] = 0;

    style_.begin(ctx,
        text((table.cell_index_[1] & 1) != 0 ? "odd-row" : "even-row"),
        WIDGET_NORMAL, SCOPED_SUBSTYLE_NO_PATH_SEPARATOR);
    alia_if (table.cell_index_[1] == 1)
    {
        special_style_.begin(ctx, text("first-row"), WIDGET_NORMAL,
            SCOPED_SUBSTYLE_NO_PATH_SEPARATOR);
    }
    alia_end

    grid_row_.begin(table.grid_, layout_spec);
}
void table_row::end()
{
    if (table_)
    {
        grid_row_.end();
        special_style_.end();
        style_.end();
        table_ = 0;
    }
}

void table_cell::begin(table_row& row, layout const& layout_spec)
{
    row_ = &row;
    table& table = *row.table_;
    ui_context& ctx = *table.ctx_;

    ++table.cell_index_[0];
    alia_if (table.cell_index_[0] == 1)
    {
        special_style_.begin(ctx, text("first-column"), WIDGET_NORMAL,
            SCOPED_SUBSTYLE_NO_PATH_SEPARATOR);
    }
    alia_end
    panel_.begin(ctx,
        text((table.cell_index_[0] & 1) != 0 ? "odd-column" : "even-column"),
        add_default_padding(layout_spec, UNPADDED));
}
void table_cell::end()
{
    if (row_)
    {
        panel_.end();
        special_style_.end();
        row_ = 0;
    }
}

}
