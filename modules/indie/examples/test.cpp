#include <alia/core/flow/data_graph.hpp>
#include <alia/core/flow/events.hpp>
#include <alia/indie.hpp>
#include <alia/indie/layout/containers/simple.hpp>
#include <alia/indie/layout/containers/utilities.hpp>
#include <alia/indie/layout/library.hpp>
#include <alia/indie/layout/logic/flow.hpp>
#include <alia/indie/layout/logic/linear.hpp>
#include <alia/indie/layout/spacer.hpp>
#include <alia/indie/layout/utilities.hpp>
#include <alia/indie/system/api.hpp>
#include <alia/indie/system/input_constants.hpp>
#include <alia/indie/utilities/hit_testing.hpp>
#include <alia/indie/utilities/keyboard.hpp>
#include <alia/indie/utilities/mouse.hpp>
#include <alia/indie/widget.hpp>

#include <color/color.hpp>

#include <cmath>

#include <include/core/SkColor.h>
#include <include/core/SkPictureRecorder.h>
#include <include/core/SkTextBlob.h>
#include <limits>
#include <modules/skshaper/include/SkShaper.h>

#include "alia/core/flow/top_level.hpp"
#include "alia/indie/context.hpp"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/TextStyle.h"
#include "modules/skparagraph/src/ParagraphBuilderImpl.h"
#include "modules/skparagraph/src/ParagraphImpl.h"

using namespace alia;
using namespace alia::indie;

// std::unique_ptr<SkShaper> the_shaper;

struct click_event : targeted_event
{
};

struct box_node : indie::layout_leaf
{
    void
    render(SkCanvas& canvas) override
    {
        auto const& region = this->assignment().region;

        double blend_factor = 0;

        if (indie::is_click_in_progress(*sys_, this, indie::mouse_button::LEFT)
            || is_pressed(keyboard_click_state_))
        {
            blend_factor = 0.4;
        }
        else if (is_click_possible(*sys_, this))
        {
            blend_factor = 0.2;
        }

        ::color::rgb<std::uint8_t> c;
        if (state_)
        {
            c = ::color::rgb<std::uint8_t>({0x00, 0x00, 0xff});
        }
        else
        {
            c = ::color::rgb<std::uint8_t>(
                {SkColorGetR(color_),
                 SkColorGetG(color_),
                 SkColorGetB(color_)});
        }
        if (blend_factor != 0)
        {
            ::color::yiq<std::uint8_t> color;
            color = c;
            ::color::yiq<std::uint8_t> white = ::color::constant::white_t{};
            ::color::yiq<std::uint8_t> mix
                = ::color::operation::mix(color, blend_factor, white);
            c = mix;
        }

        SkPaint paint;
        paint.setColor(SkColorSetARGB(
            0xff,
            ::color::get::red(c),
            ::color::get::green(c),
            ::color::get::blue(c)));
        SkRect rect;
        rect.fLeft = SkScalar(region.corner[0]);
        rect.fTop = SkScalar(region.corner[1]);
        rect.fRight = SkScalar(region.corner[0] + region.size[0]);
        rect.fBottom = SkScalar(region.corner[1] + region.size[1]);
        canvas.drawRect(rect, paint);

        if (indie::widget_has_focus(*sys_, this))
        {
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setStrokeWidth(4);
            paint.setColor(SK_ColorBLACK);
            canvas.drawRect(rect, paint);
        }
    }

    void
    hit_test(indie::hit_test_base& test, vector<2, double> const& point)
        const override
    {
        if (is_inside(this->assignment().region, vector<2, float>(point)))
        {
            switch (test.type)
            {
                case indie::hit_test_type::MOUSE: {
                    static_cast<indie::mouse_hit_test&>(test).result
                        = indie::mouse_hit_test_result{
                            externalize(this),
                            indie::mouse_cursor::POINTER,
                            this->assignment().region,
                            ""};
                    break;
                }
                case indie::hit_test_type::WHEEL: {
                    static_cast<indie::wheel_hit_test&>(test).result
                        = externalize(this);
                    break;
                }
            }
        }
    }

    void
    process_input(indie::event_context ctx) override
    {
        indie::add_to_focus_order(ctx, this);
        if (detect_click(ctx, this, indie::mouse_button::LEFT))
        {
            click_event event;
            dispatch_targeted_event(*sys_, event, this->id_);
            // state_ = !state_;
            // advance_focus(get_system(ctx));
        }
        // if (detect_key_press(ctx, this, indie::key_code::SPACE))
        // {
        //     state_ = !state_;
        //     // advance_focus(get_system(ctx));
        // }
        if (detect_keyboard_click(ctx, keyboard_click_state_, this))
        {
            state_ = !state_;
        }
    }

    // external_component_id
    // identity() const
    // {
    //     return id_;
    // }

    indie::system* sys_;
    external_component_id id_;
    bool state_ = false;
    SkColor color_ = SK_ColorWHITE;
    indie::keyboard_click_state keyboard_click_state_;
    // sk_sp<SkPicture> picture_;
};

void
do_box(
    indie::context ctx,
    SkColor color,
    action<> on_click,
    layout const& layout_spec = layout(TOP | LEFT | PADDED))
{
    std::shared_ptr<box_node>* node_ptr;
    if (get_cached_data(ctx, &node_ptr))
    {
        *node_ptr = std::make_shared<box_node>();
        (*node_ptr)->sys_ = &get_system(ctx);
        (*node_ptr)->color_ = color;
    }

    auto& node = **node_ptr;

    auto id = get_component_id(ctx);

    if (is_refresh_event(ctx))
    {
        node.refresh_layout(
            get<indie::traversal_tag>(ctx).layout,
            layout_spec,
            leaf_layout_requirements(make_layout_vector(40, 40), 0, 0));
        add_layout_node(get<indie::traversal_tag>(ctx).layout, &node);

        node.id_ = externalize(id);

        // if (color != node.color_)
        // {
        //     SkPictureRecorder recorder;
        //     SkRect bounds;
        //     bounds.fLeft = 0;
        //     bounds.fTop = 0;
        //     bounds.fRight = 100;
        //     bounds.fBottom = 100;
        //     SkCanvas* canvas = recorder.beginRecording(bounds);

        //     {
        //         SkPaint paint;
        //         paint.setColor(color);
        //         SkRect rect;
        //         rect.fLeft = 0;
        //         rect.fTop = 0;
        //         rect.fRight = 100;
        //         rect.fBottom = 100;
        //         canvas->drawRect(rect, paint);
        //     }

        //     node.picture_ = recorder.finishRecordingAsPicture();
        // }
    }
    click_event* click;
    if (detect_targeted_event(ctx, id, &click))
    {
        perform_action(on_click);
        abort_traversal(ctx);
    }
}

struct text_node : indie::layout_leaf
{
    void
    render(SkCanvas& canvas) override
    {
        auto const& region = this->assignment().region;

        SkRect bounds;
        bounds.fLeft = SkScalar(region.corner[0]);
        bounds.fTop = SkScalar(region.corner[1]);
        bounds.fRight = SkScalar(region.corner[0] + region.size[0]);
        bounds.fBottom = SkScalar(region.corner[1] + region.size[1]);
        if (canvas.quickReject(bounds))
            return;
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SK_ColorBLACK);
        // paragraph->paint(&canvas, region.corner[0], region.corner[1]);
        // canvas.drawColor(SK_ColorBLACK);
        canvas.drawTextBlob(
            text_blob_.get(), region.corner[0], region.corner[1], paint);
    }

    void
    hit_test(indie::hit_test_base&, vector<2, double> const&) const override
    {
    }

    void
    process_input(indie::event_context) override
    {
    }

    indie::system* sys_;
    // std::unique_ptr<skia::textlayout::Paragraph> paragraph;
    sk_sp<SkTextBlob> text_blob_;
};

void
do_text(
    indie::context ctx,
    readable<std::string> text,
    layout const& layout_spec = default_layout)
{
    std::shared_ptr<text_node>* node_ptr;
    if (get_cached_data(ctx, &node_ptr))
    {
        *node_ptr = std::make_shared<text_node>();
        (*node_ptr)->sys_ = &get_system(ctx);

#if 0
        SkPaint paint;
        paint.setAntiAlias(true);
        paint.setColor(SK_ColorBLACK);
        skia::textlayout::TextStyle defaultStyle;
        defaultStyle.setForegroundColor(paint);
        skia::textlayout::ParagraphStyle paraStyle;

        defaultStyle.setFontSize(SkScalar(24));
        paraStyle.setTextStyle(defaultStyle);
        skia::textlayout::ParagraphBuilderImpl builder(
            paraStyle, get_system(ctx).font_collection);

        skia::textlayout::TextStyle style;
        style.setFontFamilies({SkString("Roboto")});
        SkFontStyle fontStyle(
            false ? SkFontStyle::Weight::kBold_Weight
                  : SkFontStyle::Weight::kNormal_Weight,
            SkFontStyle::Width::kNormal_Width,
            false ? SkFontStyle::Slant::kItalic_Slant
                  : SkFontStyle::Slant::kUpright_Slant);
        style.setForegroundColor(paint);
        style.setFontStyle(fontStyle);
        style.setFontSize(24);
        // if (false)
        // {
        //     style.addShadow(skia::textlayout::TextShadow(
        //         SK_ColorBLACK, SkPoint::Make(5, 5), 2));
        // }
        // bool test
        //     = (skia::textlayout::TextDecoration) 0
        //       != skia::textlayout::TextDecoration::kNoDecoration;
        // if (test)
        // {
        //     style.setDecoration((TextDecoration) decoration);
        //     style.setDecorationStyle(std::get<7>(para));
        //     style.setDecorationColor(std::get<5>(para));
        // }
        builder.pushStyle(style);
        builder.addText(read_signal(text).c_str());
        builder.pop();

        (*node_ptr)->paragraph = builder.Build();
        (*node_ptr)->paragraph->layout(
            std::numeric_limits<SkScalar>::infinity());
#else
        (*node_ptr)->text_blob_ = SkTextBlob::MakeFromString(
            read_signal(text).c_str(), SkFont(nullptr, 24));
#endif
    }

    auto& node = **node_ptr;

    // auto id = get_component_id(ctx);

    if (is_refresh_event(ctx))
    {
        node.refresh_layout(
            get<indie::traversal_tag>(ctx).layout,
            layout_spec,
            leaf_layout_requirements(make_layout_vector(80, 40), 0, 0));
        add_layout_node(get<indie::traversal_tag>(ctx).layout, &node);

        // node.id_ = externalize(id);

        // if (color != node.color_)
        // {
        //     SkPictureRecorder recorder;
        //     SkRect bounds;
        //     bounds.fLeft = 0;
        //     bounds.fTop = 0;
        //     bounds.fRight = 100;
        //     bounds.fBottom = 100;
        //     SkCanvas* canvas = recorder.beginRecording(bounds);

        //     {
        //         SkPaint paint;
        //         paint.setColor(color);
        //         SkRect rect;
        //         rect.fLeft = 0;
        //         rect.fTop = 0;
        //         rect.fRight = 100;
        //         rect.fBottom = 100;
        //         canvas->drawRect(rect, paint);
        //     }

        //     node.picture_ = recorder.finishRecordingAsPicture();
        // }
    }
}

namespace alia { namespace indie {

struct scoped_flow_layout : simple_scoped_layout<flow_layout_logic>
{
    using simple_scoped_layout::simple_scoped_layout;

    void
    begin(context ctx, layout const& layout_spec = default_layout)
    {
        // With a flow layout, we want to have the layout itself
        // always fill the horizontal space and use the requested X
        // alignment to position the individual rows in the flow.
        auto adjusted_layout_spec = add_default_padding(layout_spec, PADDED);
        layout_flag_set x_alignment = FILL_X;
        if ((layout_spec.flags.code & 0x3) != 0)
        {
            x_alignment.code
                = adjusted_layout_spec.flags.code & X_ALIGNMENT_MASK_CODE;
            adjusted_layout_spec.flags.code &= ~X_ALIGNMENT_MASK_CODE;
            adjusted_layout_spec.flags.code |= FILL_X_CODE;
        }
        flow_layout_logic* logic;
        get_simple_layout_container(
            get_layout_traversal(ctx),
            get_data_traversal(ctx),
            &container_,
            &logic,
            adjusted_layout_spec);
        logic->x_alignment = x_alignment;
        slc_.begin(get_layout_traversal(ctx), container_);
    }
};

struct nonuniform_grid_tag
{
};
struct uniform_grid_tag
{
};

// This structure stores the layout requirements for the columns in a grid.
template<class Uniformity>
struct grid_column_requirements
{
};
// In nonuniform grids, each column has its own requirements.
template<>
struct grid_column_requirements<nonuniform_grid_tag>
{
    std::vector<layout_requirements> columns;
};
// In uniform grids, the columns all share the same requirements.
template<>
struct grid_column_requirements<uniform_grid_tag>
{
    size_t n_columns;
    layout_requirements requirements;
};

// Get the number of columns.
static size_t
get_column_count(grid_column_requirements<nonuniform_grid_tag> const& x)
{
    return x.columns.size();
}
static size_t
get_column_count(grid_column_requirements<uniform_grid_tag> const& x)
{
    return x.n_columns;
}

// Reset the column requirements.
static void
clear_requirements(grid_column_requirements<nonuniform_grid_tag>& x)
{
    x.columns.clear();
}
static void
clear_requirements(grid_column_requirements<uniform_grid_tag>& x)
{
    x.n_columns = 0;
    x.requirements = layout_requirements{0, 0, 0, 1};
}

// Add the requirements for a column.
static void
add_requirements(
    grid_column_requirements<nonuniform_grid_tag>& x,
    layout_requirements const& addition)
{
    x.columns.push_back(addition);
}
static void
add_requirements(
    grid_column_requirements<uniform_grid_tag>& x,
    layout_requirements const& addition)
{
    ++x.n_columns;
    fold_in_requirements(x.requirements, addition);
}

// Fold the second set of requirements into the first.
static void
fold_in_requirements(
    grid_column_requirements<nonuniform_grid_tag>& x,
    grid_column_requirements<nonuniform_grid_tag> const& y)
{
    size_t n_columns = get_column_count(y);
    if (get_column_count(x) < n_columns)
        x.columns.resize(n_columns, layout_requirements{0, 0, 0, 0});
    for (size_t i = 0; i != n_columns; ++i)
    {
        layout_requirements& xi = x.columns[i];
        layout_requirements const& yi = y.columns[i];
        fold_in_requirements(xi, yi);
        if (xi.growth_factor < yi.growth_factor)
            xi.growth_factor = yi.growth_factor;
    }
}
static void
fold_in_requirements(
    grid_column_requirements<uniform_grid_tag>& x,
    grid_column_requirements<uniform_grid_tag> const& y)
{
    if (x.n_columns < y.n_columns)
        x.n_columns = y.n_columns;
    fold_in_requirements(x.requirements, y.requirements);
}

// Get the requirements for the nth column.
inline layout_requirements const&
get_column_requirements(
    grid_column_requirements<nonuniform_grid_tag> const& x, size_t n)
{
    return x.columns[n];
}
inline layout_requirements const&
get_column_requirements(
    grid_column_requirements<uniform_grid_tag> const& x, size_t /*n*/)
{
    return x.requirements;
}

template<class Uniformity>
struct grid_row_container;

template<class Uniformity>
struct cached_grid_vertical_requirements
{
};

template<>
struct cached_grid_vertical_requirements<uniform_grid_tag>
{
    calculated_layout_requirements requirements;
    counter_type last_update = 1;
};

template<class Uniformity>
struct grid_data
{
    // the container that contains the whole grid
    widget_container* container = nullptr;

    // list of rows in the grid
    grid_row_container<Uniformity>* rows = nullptr;

    // spacing between columns
    layout_scalar column_spacing;

    // requirements for the columns
    grid_column_requirements<Uniformity> requirements;
    counter_type last_content_query = 0;

    // cached vertical requirements
    cached_grid_vertical_requirements<Uniformity> vertical_requirements_cache;

    // cached assignments
    std::vector<layout_scalar> assignments;
    counter_type last_assignments_update;
};

template<class Uniformity>
void
refresh_grid(
    layout_traversal<widget_container, widget>& traversal,
    grid_data<Uniformity>& data)
{
    if (traversal.is_refresh_pass)
    {
        // Reset the row list.
        data.rows = 0;
    }
}

template<class Uniformity>
void
refresh_grid_row(
    layout_traversal<widget_container, widget>& traversal,
    grid_data<Uniformity>& grid,
    grid_row_container<Uniformity>& row,
    layout const& layout_spec)
{
    // Add this row to the grid's list of rows.
    // It doesn't matter what order the list is in, and adding the row to the
    // front of the list is easier.
    if (traversal.is_refresh_pass)
    {
        row.next = grid.rows;
        grid.rows = &row;
        row.grid = &grid;
    }

    update_layout_cacher(traversal, row.cacher, layout_spec, FILL | UNPADDED);
}

struct scoped_grid_layout
{
    scoped_grid_layout()
    {
    }

    template<class Context>
    scoped_grid_layout(
        Context& ctx,
        layout const& layout_spec = default_layout,
        absolute_length const& column_spacing = absolute_length(0, PIXELS))
    {
        begin(ctx, layout_spec, column_spacing);
    }

    ~scoped_grid_layout()
    {
        end();
    }

    template<class Context>
    void
    begin(
        Context& ctx,
        layout const& layout_spec = default_layout,
        absolute_length const& column_spacing = absolute_length(0, PIXELS))
    {
        concrete_begin(
            get_layout_traversal(ctx),
            get_data_traversal(ctx),
            layout_spec,
            column_spacing);
    }

    void
    end()
    {
        container_.end();
    }

 private:
    void
    concrete_begin(
        layout_traversal<widget_container, widget>& traversal,
        data_traversal& data,
        layout const& layout_spec,
        absolute_length const& column_spacing);

    friend struct scoped_grid_row;

    scoped_layout_container container_;
    layout_traversal<widget_container, widget>* traversal_;
    data_traversal* data_traversal_;
    grid_data<nonuniform_grid_tag>* data_;
};

void
scoped_grid_layout::concrete_begin(
    layout_traversal<widget_container, widget>& traversal,
    data_traversal& data,
    layout const& layout_spec,
    absolute_length const& column_spacing)
{
    traversal_ = &traversal;
    data_traversal_ = &data;

    get_cached_data(data, &data_);
    refresh_grid(traversal, *data_);

    layout_container_widget<simple_layout_container<column_layout_logic>>*
        container;
    column_layout_logic* logic;
    get_simple_layout_container(
        traversal, data, &container, &logic, layout_spec);
    container_.begin(traversal, container);

    data_->container = container;

    layout_scalar resolved_spacing = as_layout_size(
        resolve_absolute_length(traversal, 0, column_spacing));
    detect_layout_change(traversal, &data_->column_spacing, resolved_spacing);
}

struct scoped_grid_row
{
    scoped_grid_row()
    {
    }
    scoped_grid_row(
        indie::context ctx,
        scoped_grid_layout const& g,
        layout const& layout_spec = default_layout)
    {
        begin(ctx, g, layout_spec);
    }
    ~scoped_grid_row()
    {
        end();
    }
    void
    begin(
        indie::context ctx,
        scoped_grid_layout const& g,
        layout const& layout_spec = default_layout);
    void
    end();

 private:
    scoped_layout_container container_;
};

template<class Uniformity>
struct grid_row_container : widget_container
{
    void
    render(SkCanvas& canvas) override
    {
        auto const& region = get_assignment(this->cacher).region;
        SkRect bounds;
        bounds.fLeft = SkScalar(region.corner[0]);
        bounds.fTop = SkScalar(region.corner[1]);
        bounds.fRight = SkScalar(region.corner[0] + region.size[0]);
        bounds.fBottom = SkScalar(region.corner[1] + region.size[1]);
        if (!canvas.quickReject(bounds))
        {
            canvas.save();
            auto const& offset = region.corner;
            canvas.translate(offset[0], offset[1]);
            indie::render_children(canvas, *this);
            canvas.restore();
        }
    }

    void
    hit_test(
        hit_test_base& test, vector<2, double> const& point) const override
    {
        auto const& region = get_assignment(this->cacher).region;
        if (is_inside(region, vector<2, float>(point)))
        {
            auto local_point = point - vector<2, double>(region.corner);
            for (widget* node = this->widget_container::children; node;
                 node = node->next)
            {
                node->hit_test(test, local_point);
            }
        }
    }

    void
    process_input(event_context) override
    {
    }

    // implementation of layout interface
    layout_requirements
    get_horizontal_requirements() override;
    layout_requirements
    get_vertical_requirements(layout_scalar assigned_width) override;
    void
    set_relative_assignment(
        relative_layout_assignment const& assignment) override;

    void
    record_content_change(
        layout_traversal<widget_container, widget>& traversal) override;
    void
    record_self_change(layout_traversal<widget_container, widget>& traversal);

    layout_cacher cacher;

    // cached requirements for cells within this row
    grid_column_requirements<Uniformity> requirements;
    counter_type last_content_query = 0;

    // reference to the data for the grid that this row belongs to
    grid_data<Uniformity>* grid = nullptr;

    // next row in this grid
    grid_row_container* next = nullptr;
};

// Update the requirements for a grid's columns by querying its contents.
template<class Uniformity>
void
update_grid_column_requirements(grid_data<Uniformity>& grid)
{
    // Only update if something in the grid has changed since the last update.
    if (grid.last_content_query != grid.container->last_content_change)
    {
        // Clear the requirements for the grid and recompute them
        // by iterating through the rows and folding each row's
        // requirements into the main grid requirements.
        clear_requirements(grid.requirements);
        for (grid_row_container<Uniformity>* row = grid.rows; row;
             row = row->next)
        {
            // Again, only update if something in the row has
            // changed.
            if (row->last_content_query != row->last_content_change)
            {
                clear_requirements(row->requirements);
                for (widget* child = row->widget_container::children; child;
                     child = child->next)
                {
                    layout_requirements x
                        = child->get_horizontal_requirements();
                    add_requirements(row->requirements, x);
                }
                row->last_content_query = row->last_content_change;
            }
            fold_in_requirements(grid.requirements, row->requirements);
        }
        grid.last_content_query = grid.container->last_content_change;
    }
}

template<class Uniformity>
layout_scalar
get_required_width(grid_data<Uniformity> const& grid)
{
    size_t n_columns = get_column_count(grid.requirements);
    layout_scalar width = 0;
    for (size_t i = 0; i != n_columns; ++i)
        width += get_column_requirements(grid.requirements, i).size;
    if (n_columns > 0)
        width += grid.column_spacing * layout_scalar(n_columns - 1);
    return width;
}

template<class Uniformity>
float
get_total_growth(grid_data<Uniformity> const& grid)
{
    size_t n_columns = get_column_count(grid.requirements);
    float growth = 0;
    for (size_t i = 0; i != n_columns; ++i)
        growth += get_column_requirements(grid.requirements, i).growth_factor;
    return growth;
}

template<class Uniformity>
layout_requirements
grid_row_container<Uniformity>::get_horizontal_requirements()
{
    return cache_horizontal_layout_requirements(
        cacher, grid->container->last_content_change, [&] {
            update_grid_column_requirements(*grid);
            return calculated_layout_requirements{
                get_required_width(*grid), 0, 0};
        });
}

template<class Uniformity>
std::vector<layout_scalar> const&
calculate_column_assignments(
    grid_data<Uniformity>& grid, layout_scalar assigned_width)
{
    if (grid.last_assignments_update != grid.container->last_content_change)
    {
        update_grid_column_requirements(grid);
        size_t n_columns = get_column_count(grid.requirements);
        grid.assignments.resize(n_columns);
        layout_scalar required_width = get_required_width(grid);
        float total_growth = get_total_growth(grid);
        layout_scalar extra_width = assigned_width - required_width;
        for (size_t i = 0; i != n_columns; ++i)
        {
            layout_scalar width
                = get_column_requirements(grid.requirements, i).size;
            if (total_growth != 0)
            {
                float growth_factor
                    = get_column_requirements(grid.requirements, i)
                          .growth_factor;
                layout_scalar extra = round_to_layout_scalar(
                    (growth_factor / total_growth) * extra_width);
                extra_width -= extra;
                total_growth -= growth_factor;
                width += extra;
            }
            grid.assignments[i] = width;
        }
        grid.last_assignments_update = grid.container->last_content_change;
    }
    return grid.assignments;
}

calculated_layout_requirements
calculate_grid_row_vertical_requirements(
    grid_data<nonuniform_grid_tag>& grid,
    grid_row_container<nonuniform_grid_tag>& row,
    layout_scalar assigned_width)
{
    std::vector<layout_scalar> const& column_widths
        = calculate_column_assignments(grid, assigned_width);
    calculated_layout_requirements requirements{0, 0, 0};
    size_t column_index = 0;
    walk_layout_nodes(row.children, [&](layout_node_interface& node) {
        fold_in_requirements(
            requirements,
            node.get_vertical_requirements(column_widths[column_index]));
        ++column_index;
    });
    return requirements;
}

calculated_layout_requirements
calculate_grid_row_vertical_requirements(
    grid_data<uniform_grid_tag>& grid,
    grid_row_container<uniform_grid_tag>& /*row*/,
    layout_scalar assigned_width)
{
    named_block nb;
    auto& cache = grid.vertical_requirements_cache;
    if (cache.last_update != grid.container->last_content_change)
    {
        update_grid_column_requirements(grid);

        std::vector<layout_scalar> const& widths
            = calculate_column_assignments(grid, assigned_width);

        calculated_layout_requirements& grid_requirements = cache.requirements;
        grid_requirements = calculated_layout_requirements{0, 0, 0};
        for (grid_row_container<uniform_grid_tag>* row = grid.rows; row;
             row = row->next)
        {
            size_t column_index = 0;
            walk_layout_nodes(row->children, [&](layout_node_interface& node) {
                fold_in_requirements(
                    grid_requirements,
                    node.get_vertical_requirements(widths[column_index]));
                ++column_index;
            });
        }

        cache.last_update = grid.container->last_content_change;
    }
    return cache.requirements;
}

template<class Uniformity>
layout_requirements
grid_row_container<Uniformity>::get_vertical_requirements(
    layout_scalar assigned_width)
{
    return cache_vertical_layout_requirements(
        cacher, grid->container->last_content_change, assigned_width, [&] {
            return calculate_grid_row_vertical_requirements(
                *grid, *this, assigned_width);
        });
}

template<class Uniformity>
void
set_grid_row_relative_assignment(
    grid_data<Uniformity>& grid,
    widget* children,
    layout_vector const& assigned_size,
    layout_scalar assigned_baseline_y)
{
    std::vector<layout_scalar> const& column_widths
        = calculate_column_assignments(grid, assigned_size[0]);
    size_t n = 0;
    layout_vector p = make_layout_vector(0, 0);
    for (widget* i = children; i; i = i->next, ++n)
    {
        layout_scalar this_width = column_widths[n];
        i->set_relative_assignment(relative_layout_assignment{
            layout_box(p, make_layout_vector(this_width, assigned_size[1])),
            assigned_baseline_y});
        p[0] += this_width + grid.column_spacing;
    }
}

template<class Uniformity>
void
grid_row_container<Uniformity>::set_relative_assignment(
    relative_layout_assignment const& assignment)
{
    update_relative_assignment(
        *this,
        cacher,
        grid->container->last_content_change,
        assignment,
        [&](auto const& resolved_assignment) {
            set_grid_row_relative_assignment(
                *grid,
                widget_container::children,
                resolved_assignment.region.size,
                resolved_assignment.baseline_y);
        });
}

template<class Uniformity>
void
grid_row_container<Uniformity>::record_content_change(
    layout_traversal<widget_container, widget>& traversal)
{
    if (this->last_content_change != traversal.refresh_counter)
    {
        this->last_content_change = traversal.refresh_counter;
        if (this->parent)
            this->parent->record_content_change(traversal);
        for (grid_row_container<Uniformity>* row = this->grid->rows; row;
             row = row->next)
        {
            row->record_self_change(traversal);
        }
    }
}

template<class Uniformity>
void
grid_row_container<Uniformity>::record_self_change(
    layout_traversal<widget_container, widget>& traversal)
{
    if (this->last_content_change != traversal.refresh_counter)
    {
        this->last_content_change = traversal.refresh_counter;
        if (this->parent)
            this->parent->record_content_change(traversal);
    }
}

void
scoped_grid_row::begin(
    indie::context, scoped_grid_layout const& grid, layout const& layout_spec)
{
    layout_traversal<widget_container, widget>& traversal = *grid.traversal_;

    grid_row_container<nonuniform_grid_tag>* row;
    if (get_cached_data(*grid.data_traversal_, &row))
        initialize(traversal, *row);

    refresh_grid_row(traversal, *grid.data_, *row, layout_spec);

    container_.begin(traversal, row);
}

void
scoped_grid_row::end()
{
    container_.end();
}

}} // namespace alia::indie

void
my_ui(indie::context ctx)
{
    // static indie::layout_container_widget<column_layout> container;
    // static indie::simple_container_widget container;
    // indie::scoped_widget_container container_scope;
    // if (is_refresh_event(ctx))
    // {
    //     container_scope.begin(
    //         get<indie::traversal_tag>(ctx).widgets, &container);
    // }

    indie::scoped_column column(ctx, GROW | PADDED);

    auto show_text = get_state(ctx, false);

    do_box(ctx, SK_ColorLTGRAY, actions::toggle(show_text));
    do_spacer(ctx, height(100, PIXELS));

    {
        indie::scoped_grid_layout grid(ctx);
        {
            indie::scoped_grid_row row(ctx, grid);
            do_box(ctx, SK_ColorMAGENTA, actions::noop(), width(200, PIXELS));
            do_box(ctx, SK_ColorMAGENTA, actions::noop(), width(200, PIXELS));
        }
        {
            indie::scoped_grid_row row(ctx, grid);
            do_box(ctx, SK_ColorLTGRAY, actions::noop());
            do_box(ctx, SK_ColorLTGRAY, actions::noop());
        }
    }
    do_spacer(ctx, height(100, PIXELS));

    {
        indie::scoped_flow_layout flow(ctx, GROW | UNPADDED);

        for (int i = 0; i != 100; ++i)
        {
            alia_if(show_text)
            {
                do_text(ctx, alia::printf(ctx, "text%i", i));
            }
            alia_end

            {
                indie::scoped_column col(ctx);

                do_box(
                    ctx, SK_ColorMAGENTA, actions::noop(), width(100, PIXELS));

                // color::yiq<std::uint8_t> y1 =
                // ::color::constant::blue_t{};
                // color::yiq<std::uint8_t> y2 =
                // ::color::constant::red_t{};
                // color::yiq<std::uint8_t> yr =
                // color::operation::mix(
                //     y1,
                //     std::max(
                //         0.0,
                //         std::min(
                //             1.0,
                //             std::fabs(std::sin(
                //                 get_raw_animation_tick_count(ctx)
                //                 / 1000.0)))),
                //     y2);
                // color::rgb<std::uint8_t> r(yr);
                // color::rgb<std::uint8_t> r(y1);

                do_box(ctx, SK_ColorLTGRAY, actions::noop());
            }

            {
                indie::scoped_column col(ctx);

                static SkColor clicky_color = SK_ColorRED;
                // event_handler<indie::mouse_button_event>(
                //     ctx, [&](auto, auto&) { clicky_color =
                //     SK_ColorBLUE; });
                do_box(ctx, clicky_color, actions::noop());

                do_box(ctx, SK_ColorDKGRAY, actions::noop());

                do_box(ctx, SK_ColorGRAY, actions::noop());
            }
        }
    }
}
