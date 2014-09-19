#include <alia/ui/utilities/validation.hpp>

namespace alia {

void add_validation_error_report(
    dataless_ui_context& ctx, validation_error_report* report)
{
    validation_error_reporting_context* reporting = ctx.validation.reporting;
    if (reporting)
    {
        report->next = reporting->data->reports;
        reporting->data->reports = report;
    }
}

void do_validation_report(ui_context& ctx, validation_error_report* reports)
{
    alia_if (reports)
    {
        do_validation_report(ctx, reports->next);
        panel p(ctx, text("validation-error-panel"));
        do_paragraph(ctx, in(*reports->message));
    }
    alia_end
}

void scoped_error_reporting_context::begin(
    dataless_ui_context& ctx, validation_error_reporting_data* reporting)
{
    ctx_ = &ctx;

    // On refresh passes, the report list is rebuilt.
    if (is_refresh_pass(ctx))
        reporting->reports = 0;

    old_reporting_ = ctx.validation.reporting;

    reporting_.data = reporting;
    ctx.validation.reporting = &reporting_;
}
void scoped_error_reporting_context::end()
{
    if (ctx_)
    {
        ctx_->validation.reporting = old_reporting_;
        ctx_ = 0;
    }
}

}
