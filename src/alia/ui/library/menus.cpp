#include <alia/ui/api.hpp>
#include <alia/ui/utilities.hpp>

namespace alia {

namespace {

void record_change(ui_context& ctx)
{
    menu_container* container = ctx.menu.active_container;
    counter_type refresh_counter = get_refresh_counter(ctx);
    while (container && container->last_change != refresh_counter)
    {
        container->last_change = refresh_counter;
        container = container->parent;
    }
}

template<class Value>
void detect_change(ui_context& ctx, Value* stored_value, Value const& value)
{
    if (*stored_value != value)
    {
        record_change(ctx);
        *stored_value = value;
    }
}

void set_next_node(ui_context& ctx, menu_node* next)
{
    detect_change(ctx, ctx.menu.next_ptr, next);
}

void detect_label_change(ui_context& ctx, keyed_data<string>* storage,
    accessor<string> const& value)
{
    refresh_keyed_data(*storage, value.id());
    if (!is_valid(*storage))
        set(*storage, get(value));
}

}

void scoped_menu_container::begin(ui_context& ctx, menu_container* container)
{
    if (is_refresh_pass(ctx))
    {
        ctx_ = &ctx;

        container->parent = ctx.menu.active_container;
        ctx.menu.active_container = container;

        if (ctx.menu.next_ptr)
            set_next_node(ctx, container);
        ctx.menu.next_ptr = &container->children;
    }
}
void scoped_menu_container::end()
{
    if (ctx_)
    {
        ui_context& ctx = *ctx_;
        ctx_ = 0;

        menu_container* container = ctx.menu.active_container;
        ctx.menu.active_container = container->parent;

        set_next_node(ctx, 0);
        ctx.menu.next_ptr = &container->next;
    }
}

void submenu::begin(ui_context& ctx, accessor<string> const& label,
    accessor<bool> const& enabled)
{
    submenu_node* node;
    if (get_cached_data(ctx, &node))
        node->type = SUBMENU_NODE;

    if (is_refresh_pass(ctx) && is_gettable(label) && is_gettable(enabled))
    {
        scoping_.begin(ctx, node);
        detect_label_change(ctx, &node->label, label);
        detect_change(ctx, &node->enabled, get(enabled));
    }
}
void submenu::end()
{
    scoping_.end();
}

namespace {

bool detect_menu_item_selection(ui_context& ctx, menu_item_node* node)
{
    menu_item_selection_event* event =
        dynamic_cast<menu_item_selection_event*>(ctx.event);
    return event && event->target == node;
}

}

bool do_menu_option(ui_context& ctx, accessor<string> const& label,
    accessor<bool> const& enabled)
{
    menu_item_node* node;
    if (get_cached_data(ctx, &node))
        node->type = MENU_ITEM_NODE;

    if (is_refresh_pass(ctx) && is_gettable(label) && is_gettable(enabled))
    {
        detect_label_change(ctx, &node->label, label);
        detect_change(ctx, &node->enabled, get(enabled));
        detect_change(ctx, &node->checked, optional<bool>());

        set_next_node(ctx, node);
        ctx.menu.next_ptr = &node->next;
    }

    return detect_menu_item_selection(ctx, node);
}

bool do_checkable_menu_option(ui_context& ctx, accessor<string> const& label,
    accessor<bool> const& checked, accessor<bool> const& enabled)
{
    menu_item_node* node;
    if (get_cached_data(ctx, &node))
        node->type = MENU_ITEM_NODE;

    if (is_refresh_pass(ctx) && is_gettable(label) && is_gettable(enabled) &&
        is_gettable(checked))
    {
        detect_label_change(ctx, &node->label, label);
        detect_change(ctx, &node->enabled, get(enabled));
        detect_change(ctx, &node->checked, some(get(checked)));

        set_next_node(ctx, node);
        ctx.menu.next_ptr = &node->next;
    }

    return detect_menu_item_selection(ctx, node);
}

void do_menu_separator(ui_context& ctx)
{
    menu_separator_node* node;
    if (get_cached_data(ctx, &node))
        node->type = MENU_SEPARATOR_NODE;

    if (is_refresh_pass(ctx))
    {
        set_next_node(ctx, node);
        ctx.menu.next_ptr = &node->next;
    }
}

void menu_bar::begin(ui_context& ctx)
{
    scoping_.begin(ctx, &ctx.system->menu_bar);
}
void menu_bar::end()
{
    scoping_.end();
}

}
