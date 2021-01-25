#include <QApplication>
#include <QWidget>

#define ALIA_IMPLEMENTATION
#include "alia.hpp"

#include "adaptor.hpp"

using namespace alia;

void
do_sandbox_ui(qt_context ctx)
{
    column_layout row(ctx);

    do_label(ctx, value("Hello, World!"));

    auto x = get_state(ctx, "");
    do_text_control(ctx, x);
    do_text_control(ctx, x);

    do_label(ctx, x);

    auto state = get_state(ctx, true);
    ALIA_IF(state)
    {
        do_label(ctx, value("Secret message!"));
    }
    ALIA_END

    do_button(ctx, x, actions::toggle(state));
    do_button(ctx, value("Toggle!"), actions::toggle(state));
}

alia::system the_system;
qt_system the_qt;

int
main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    initialize(the_qt, the_system, do_sandbox_ui);

    refresh_system(the_system);

    the_qt.window->setWindowTitle("alia Qt");
    the_qt.window->show();

    return app.exec();
}
