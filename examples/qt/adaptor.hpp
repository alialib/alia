#ifndef QT_ADAPTOR_HPP
#define QT_ADAPTOR_HPP

#include "alia.hpp"

struct qt_traversal;

ALIA_DEFINE_TAGGED_TYPE(qt_traversal_tag, qt_traversal&)

typedef alia::extend_context_type_t<alia::context, qt_traversal_tag>
    qt_context;

typedef alia::remove_context_tag_t<qt_context, alia::data_traversal_tag>
    dataless_qt_context;

void
do_label(qt_context ctx, alia::readable<std::string> text);

void
do_text_control(qt_context ctx, alia::duplex<std::string> text);

void
do_button(
    qt_context ctx, alia::readable<std::string> text, alia::action<> on_click);

struct qt_layout_container;

struct scoped_layout_container : alia::noncopyable
{
    scoped_layout_container()
    {
    }
    scoped_layout_container(qt_context ctx, qt_layout_container* container)
    {
        begin(ctx, container);
    }
    ~scoped_layout_container()
    {
        end();
    }
    void
    begin(qt_context ctx, qt_layout_container* container);
    void
    end();

 private:
    qt_traversal* traversal_ = nullptr;
};

struct column_layout : alia::noncopyable
{
    column_layout()
    {
    }
    column_layout(qt_context ctx)
    {
        begin(ctx);
    }
    ~column_layout()
    {
        end();
    }
    void
    begin(qt_context ctx);
    void
    end();

 private:
    scoped_layout_container slc_;
};

struct qt_layout_node;

class QWidget;
class QVBoxLayout;

struct qt_system
{
    alia::system* system = nullptr;

    std::function<void(qt_context)> controller;

    // the root of the application's UI tree
    qt_layout_node* root = nullptr;

    // the top-level window and layout for the UI - The entire application's UI
    // tree lives inside this.
    QWidget* window = nullptr;
    QVBoxLayout* layout = nullptr;

    void
    operator()(alia::context ctx);

    ~qt_system();
};

void
initialize(
    qt_system& qt_system,
    alia::system& alia_system,
    std::function<void(qt_context)> controller);

#endif
