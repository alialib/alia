#ifndef ALIA_COMPONENTS_SYSTEM_HPP
#define ALIA_COMPONENTS_SYSTEM_HPP

#include <functional>

#include <alia/components/context.hpp>
#include <alia/flow/data_graph.hpp>

namespace alia {

struct system
{
    data_graph data;
    std::function<void(context)> controller;
};

} // namespace alia

#endif
