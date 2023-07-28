#ifndef ALIA_INDIE_BACKENDS_GLFW_HPP
#define ALIA_INDIE_BACKENDS_GLFW_HPP

#include <memory>

#include <alia/indie/context.hpp>
#include <alia/indie/geometry.hpp>

namespace alia { namespace indie {

struct glfw_window_impl;

struct glfw_window
{
    glfw_window(
        std::string const& title,
        vector<2, unsigned> size,
        std::function<void(indie::context)> controller);
    ~glfw_window();

    void*
    handle() const;

    // TODO: Something more sophisticated.
    void
    do_main_loop();

 private:
    std::unique_ptr<glfw_window_impl> impl_;
};

}} // namespace alia::indie

#endif
