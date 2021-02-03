#ifndef ALIA_HTML_DEMO_UTILITIES_HPP
#define ALIA_HTML_DEMO_UTILITIES_HPP

#include <alia.hpp>

#include <alia/html/context.hpp>
#include <alia/html/fetch.hpp>

ALIA_DEFINE_TAGGED_TYPE(src_tag, alia::apply_signal<std::string>&)

typedef alia::extend_context_type_t<alia::html::context, src_tag> demo_context;

std::string
extract_code_snippet(std::string const& code, std::string const& tag);

void
code_snippet(demo_context ctx, char const* tag);

void
with_demo_context(
    alia::html::context vanilla_ctx,
    alia::function_view<void(demo_context)> const& content);

#endif
