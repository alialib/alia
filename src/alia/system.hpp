#ifndef ALIA_SYSTEM_HPP
#define ALIA_SYSTEM_HPP

#include <functional>

#include <alia/context.hpp>
#include <alia/data_graph.hpp>

namespace alia {

struct system
{
    data_graph data;
    std::function<void(context)> controller;
};

} // namespace alia

#endif
