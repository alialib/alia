#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>
#include <alia/ui/library/panels.hpp>

namespace alia {

struct table_cell_style_info
{
    panel_style_info panel_info;
    substyle_data substyle;
};

struct table_style_info
{
    table_cell_style_info cells[6];
    style_path_storage root_path_storage;
};

void get_table_cell_style_info(ui_context& ctx, table_cell_style_info* info,
    style_search_path const* path, string const& name)
{
    update_substyle_data(ctx, info->substyle, path, name, WIDGET_NORMAL,
        ADD_SUBSTYLE_IFF_EXISTS);
    info->panel_info = get_panel_style_info(ctx, info->substyle.state.path);
}

void get_table_style_info(ui_context& ctx, table_style_info* info,
    style_search_path const* path)
{
    get_table_cell_style_info(ctx, &info->cells[0], path, "even-row");
    get_table_cell_style_info(ctx, &info->cells[1],
        info->cells[0].substyle.state.path, "first-column");
    get_table_cell_style_info(ctx, &info->cells[2], path, "odd-row");
    get_table_cell_style_info(ctx, &info->cells[3],
        info->cells[2].substyle.state.path, "first-column");
    get_table_cell_style_info(ctx, &info->cells[4],
        info->cells[2].substyle.state.path, "first-row");
    get_table_cell_style_info(ctx, &info->cells[5],
        info->cells[4].substyle.state.path, "first-column");
}

void table::begin(ui_context& ctx, accessor<string> const& style,
    layout const& layout_spec)
{
    ctx_ = &ctx;

    grid_.begin(ctx, add_default_padding(layout_spec, PADDED));

    keyed_data<table_style_info>* style_data;
    if (get_cached_data(ctx, &style_data) || is_refresh_pass(ctx))
    {
        refresh_keyed_data(*style_data,
            combine_ids(ref(ctx.style.id), ref(&style.id())));
    }
    if (!is_valid(*style_data))
    {
        style_search_path const* path =
            add_substyle_to_path(&style_data->value.root_path_storage,
                ctx.style.path, ctx.style.path, get(style));
        get_table_style_info(ctx, &style_data->value, path);
        mark_valid(*style_data);
    }
    style_ = &get(*style_data);

    cell_index_ = make_vector<int>(1, 1);
}
void table::end()
{
    if (ctx_)
    {
        grid_.end();
        ctx_ = 0;
    }
}

void table_row::begin(table& table, layout const& layout_spec)
{
    table_ = &table;
    ui_context& ctx = *table.ctx_;
    table.cell_index_[0] = 1;
    grid_row_.begin(table.grid_, layout_spec);
}
void table_row::end()
{
    if (table_)
    {
        grid_row_.end();
        ++table_->cell_index_[1];
        table_ = 0;
    }
}

//void table_row_background::begin(table& table, layout const& layout_spec)
//{
//    table_ = &table;
//    ui_context& ctx = *table.ctx_;
//
//    int style_index =
//        (table.cell_index_[1] == 1 ? 2 : (table.cell_index_[1] % 2)) * 2;
//    table_cell_style_info const& cell_style = table.style_->cells[style_index];
//
//    custom_panel_data* panel_data;
//    get_cached_data(ctx, &panel_data);
//
//    panel_.begin(ctx, *panel_data,
//        make_custom_getter(&cell_style.panel_info,
//            ref(*cell_style.substyle.state.id)),
//        add_default_padding(layout_spec, UNPADDED));
//}
//void table_row_background::end()
//{
//    if (table_)
//    {
//        panel_.end();
//        table_ = 0;
//    }
//}

void table_cell::begin(table_row& row, layout const& layout_spec)
{
    row_ = &row;
    table& table = *row.table_;
    ui_context& ctx = *table.ctx_;

    int style_index =
        (table.cell_index_[1] == 1 ? 2 : (table.cell_index_[1] % 2)) * 2 +
        (table.cell_index_[0] == 1 ? 1 : 0);
    ++table.cell_index_[0];
    table_cell_style_info const& cell_style = table.style_->cells[style_index];

    custom_panel_data* panel_data;
    get_cached_data(ctx, &panel_data);

    panel_.begin(ctx, *panel_data,
        make_custom_getter(&cell_style.panel_info,
            ref(cell_style.substyle.state.id)),
        add_default_padding(layout_spec, UNPADDED));

    style_.begin(ctx, cell_style.substyle.state,
        &cell_style.substyle.style_info);
}
void table_cell::end()
{
    if (row_)
    {
        style_.end();
        panel_.end();
        row_ = 0;
    }
}

}
