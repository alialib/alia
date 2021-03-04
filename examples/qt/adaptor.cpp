#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

#include "adaptor.hpp"

using namespace alia;

using std::string;

struct qt_layout_container;

struct qt_layout_node : noncopyable
{
    virtual void
    update(alia::system* system, QWidget* parent, QLayout* layout)
        = 0;

    qt_layout_node* next;
    qt_layout_container* parent;
};

struct qt_layout_container : qt_layout_node
{
    qt_layout_node* children;

    virtual void
    record_change();

    qt_layout_container* parent;

    bool dirty;
};

struct qt_traversal
{
    QWidget* active_parent = nullptr;
    qt_layout_container* active_container = nullptr;
    // a pointer to the pointer that should store the next item that's added
    qt_layout_node** next_ptr = nullptr;
};

void
record_layout_change(qt_traversal& traversal)
{
    if (traversal.active_container)
        traversal.active_container->record_change();
}

void
set_next_node(qt_traversal& traversal, qt_layout_node* node)
{
    if (*traversal.next_ptr != node)
    {
        record_layout_change(traversal);
        *traversal.next_ptr = node;
    }
}

void
add_layout_node(dataless_qt_context ctx, qt_layout_node* node)
{
    qt_traversal& traversal = get<qt_traversal_tag>(ctx);
    set_next_node(traversal, node);
    traversal.next_ptr = &node->next;
}

void
record_container_change(qt_layout_container* container)
{
    while (container && !container->dirty)
    {
        container->dirty = true;
        container = container->parent;
    }
}

void
qt_layout_container::record_change()
{
    record_container_change(this);
}

void
scoped_layout_container::begin(qt_context ctx, qt_layout_container* container)
{
    refresh_handler(ctx, [&](auto ctx) {
        traversal_ = &get<qt_traversal_tag>(ctx);
        qt_traversal& traversal = *traversal_;

        set_next_node(traversal, container);
        container->parent = traversal.active_container;

        traversal.next_ptr = &container->children;
        traversal.active_container = container;
    });
}
void
scoped_layout_container::end()
{
    if (traversal_)
    {
        set_next_node(*traversal_, nullptr);

        qt_layout_container* container = traversal_->active_container;
        traversal_->next_ptr = &container->next;
        traversal_->active_container = container->parent;

        traversal_ = nullptr;
    }
}

struct qt_label : qt_layout_node
{
    QLabel* object = nullptr;
    captured_id text_id;

    void
    update(alia::system* system, QWidget* parent, QLayout* layout)
    {
        assert(object);
        if (object->parent() != parent)
            object->setParent(parent);
        layout->addWidget(object);
    }

    ~qt_label()
    {
        if (object)
            object->deleteLater();
    }
};

void
do_label(qt_context ctx, readable<string> text)
{
    auto& label = get_cached_data<qt_label>(ctx);

    refresh_handler(ctx, [&](auto ctx) {
        auto& system = get<system_tag>(ctx);

        if (!label.object)
        {
            auto& traversal = get<qt_traversal_tag>(ctx);
            auto* parent = traversal.active_parent;
            label.object = new QLabel(parent);
            if (parent->isVisible())
                label.object->show();
        }

        add_layout_node(ctx, &label);

        refresh_signal_view(
            label.text_id,
            text,
            [&](auto text) { label.object->setText(text.c_str()); },
            [&]() { label.object->setText(""); });
    });
}

struct click_event : targeted_event
{
};

struct qt_button : qt_layout_node
{
    QPushButton* object = nullptr;
    captured_id text_id;
    component_identity identity;

    void
    update(alia::system* system, QWidget* parent, QLayout* layout)
    {
        assert(object);
        if (object->parent() != parent)
            object->setParent(parent);
        layout->addWidget(object);
    }

    ~qt_button()
    {
        if (object)
            object->deleteLater();
    }
};

void
do_button(qt_context ctx, readable<string> text, action<> on_click)
{
    auto& button = get_cached_data<qt_button>(ctx);

    refresh_handler(ctx, [&](auto ctx) {
        auto& system = get<system_tag>(ctx);

        refresh_component_identity(ctx, button.identity);

        if (!button.object)
        {
            auto& traversal = get<qt_traversal_tag>(ctx);
            auto* parent = traversal.active_parent;
            button.object = new QPushButton(parent);
            if (parent->isVisible())
                button.object->show();
            QObject::connect(
                button.object,
                &QPushButton::clicked,
                // The Qt object is technically owned within both of these, so
                // I'm pretty sure it's safe to reference both.
                [&system, &button]() {
                    click_event event;
                    dispatch_targeted_event(
                        system, event, externalize(&button.identity));
                });
        }

        add_layout_node(ctx, &button);

        refresh_signal_view(
            button.text_id,
            text,
            [&](auto text) { button.object->setText(text.c_str()); },
            [&]() { button.object->setText(""); });
    });

    targeted_event_handler<click_event>(
        ctx, &button.identity, [&](auto ctx, auto& e) {
            if (action_is_ready(on_click))
            {
                perform_action(on_click);
            }
        });
}

struct value_update_event : targeted_event
{
    string value;
};

struct qt_text_control : qt_layout_node
{
    QTextEdit* object = nullptr;
    captured_id text_id;
    component_identity identity;

    void
    update(alia::system* system, QWidget* parent, QLayout* layout)
    {
        assert(object);
        if (object->parent() != parent)
            object->setParent(parent);
        layout->addWidget(object);
    }

    ~qt_text_control()
    {
        if (object)
            object->deleteLater();
    }
};

void
do_text_control(qt_context ctx, duplex<string> text)
{
    auto& widget = get_cached_data<qt_text_control>(ctx);

    refresh_handler(ctx, [&](auto ctx) {
        auto& system = get<system_tag>(ctx);

        refresh_component_identity(ctx, widget.identity);

        if (!widget.object)
        {
            auto& traversal = get<qt_traversal_tag>(ctx);
            auto* parent = traversal.active_parent;
            widget.object = new QTextEdit(parent);
            if (parent->isVisible())
                widget.object->show();
            QObject::connect(
                widget.object,
                &QTextEdit::textChanged,
                // The Qt object is technically owned within both of these, so
                // I'm pretty sure it's safe to reference both.
                [&system, &widget]() {
                    value_update_event event;
                    event.value
                        = widget.object->toPlainText().toUtf8().constData();
                    dispatch_targeted_event(
                        system, event, externalize(&widget.identity));
                });
        }

        add_layout_node(ctx, &widget);

        refresh_signal_view(
            widget.text_id,
            text,
            [&](auto text) {
                // Prevent update cycles.
                if (widget.object->toPlainText().toUtf8().constData() != text)
                    widget.object->setText(text.c_str());
            },
            [&]() {
                // Prevent update cycles.
                if (widget.object->toPlainText().toUtf8().constData() != "")
                    widget.object->setText("");
            });
    });

    targeted_event_handler<value_update_event>(
        ctx, &widget.identity, [&](auto ctx, auto& e) {
            write_signal(text, e.value);
        });
}

struct qt_column : qt_layout_container
{
    QVBoxLayout* object = nullptr;

    void
    update(alia::system* system, QWidget* parent, QLayout* layout)
    {
        if (!object)
            object = new QVBoxLayout(parent);

        layout->addItem(object);

        if (this->dirty)
        {
            while (object->takeAt(0))
                ;
            for (auto* node = children; node; node = node->next)
                node->update(system, parent, object);
            this->dirty = false;
        }
    }

    ~qt_column()
    {
        if (object)
            object->deleteLater();
    }
};

void
column_layout::begin(qt_context ctx)
{
    qt_column* column;
    get_cached_data(ctx, &column);
    slc_.begin(ctx, column);
}
void
column_layout::end()
{
    slc_.end();
}

void
qt_system::operator()(alia::context vanilla_ctx)
{
    qt_traversal traversal;
    qt_context ctx = extend_context<qt_traversal_tag>(vanilla_ctx, traversal);

    refresh_handler(ctx, [&](auto ctx) {
        traversal.next_ptr = &this->root;
        traversal.active_parent = this->window;
    });

    this->controller(ctx);

    refresh_handler(ctx, [&](auto ctx) {
        while (this->layout->takeAt(0))
            ;
        this->root->update(this->system, this->window, this->layout);
    });
}

void
initialize(
    qt_system& qt_system,
    alia::system& alia_system,
    std::function<void(qt_context)> controller)
{
    // Initialize the Qt system.
    qt_system.system = &alia_system;
    qt_system.root = 0;
    qt_system.window = new QWidget;
    qt_system.layout = new QVBoxLayout(qt_system.window);
    qt_system.window->setLayout(qt_system.layout);
    qt_system.controller = std::move(controller);

    // Hook up the Qt system to the alia system.
    initialize_system(alia_system, std::ref(qt_system));

    // Do the initial refresh.
    refresh_system(alia_system);
}

qt_system::~qt_system()
{
    window->deleteLater();
    layout->deleteLater();
}