#if 0

#include <alia.hpp>

namespace alia {

void do_message_dialog(canvas& canvas, std::string const& message,
    icon const* icon)
{
    {
        panel<row> content_panel(_style = CONTENT_STYLE,
            _alignment = FILL, _proportion = 1);

        if (icon != NULL)
            do_icon(*icon, _canvas = &canvas);

        do_text(ctx, &message, _canvas = &canvas,
            _multiline = true, _read_only = true, _border_type = NO_BORDER,
            _alignment = FILL, _proportion = 1);
    }

    do_separator(_alignment = FILL);

    {
        panel<column> button_panel(_alignment = FILL);
        row r;
        do_ok_button(_canvas = &canvas);
    }
}

}

#endif
