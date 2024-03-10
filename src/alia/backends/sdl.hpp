#ifndef ALIA_BACKENDS_SDL_HPP
#define ALIA_BACKENDS_SDL_HPP

#include <memory>

#include <alia/ui/context.hpp>
#include <alia/ui/geometry.hpp>

namespace alia {

struct sdl_window_impl;

struct sdl_window
{
    sdl_window(
        std::string const& title,
        vector<2, unsigned> size,
        std::function<void(ui_context)> controller);
    ~sdl_window();

    void*
    handle() const;

    // TODO: Something more sophisticated.
    void
    do_main_loop();

 private:
    std::unique_ptr<sdl_window_impl> impl_;
};

} // namespace alia

#endif
