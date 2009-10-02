#ifndef ALIA_MENU_SPECIFICATION_HPP
#define ALIA_MENU_SPECIFICATION_HPP

#include <vector>

namespace alia {

struct menu_spec
{
    enum item_type
    {
        MENU,
        OPTION,
        CHECKABLE_OPTION,
        SEPARATOR
    };

    struct item
    {
        // valid for all types
        item_type type;
        unsigned id;
        // valid for all types except SEPARATOR
        std::string text;
        bool enabled;
        // valid only for CHECKABLE_OPTION
        bool checked;
        // valid only for MENU
        std::vector<item> children;
    };

    std::vector<item> items;
};

}

#endif
