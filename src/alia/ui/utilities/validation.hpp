#ifndef ALIA_UI_UTILITIES_VALIDATION_HPP
#define ALIA_UI_UTILITIES_VALIDATION_HPP

#include <alia/ui/internals.hpp>

namespace alia {

struct validation_error_report
{
    string const* message;
    validation_error_report* next;
};

struct validation_error_reporting_data
{
    validation_error_report* reports;
};

struct validation_error_reporting_context
{
    validation_error_reporting_data* data;
};

void add_validation_error_report(
    dataless_ui_context& ctx, validation_error_report* report);

void do_validation_report(ui_context& ctx, validation_error_report* reports);

struct scoped_error_reporting_context
{
    scoped_error_reporting_context() : ctx_(0) {}
    scoped_error_reporting_context(
        dataless_ui_context& ctx, validation_error_reporting_data* data)
    { begin(ctx, data); }
    ~scoped_error_reporting_context() { end(); }
    void begin(
        dataless_ui_context& ctx, validation_error_reporting_data* data);
    void end();
 private:
    dataless_ui_context* ctx_;
    validation_error_reporting_context* old_reporting_;
    validation_error_reporting_context reporting_;
};

struct validation_error_detection_context
{
};

// A validation_error_handler is an accessor wrapper that catches
// validation_errors thrown by the wrapped accessor when attempting to set
// new values. The associated error message is recorded and persists until
// it's cleared, either explicitly or because the wrapped accessor acquires a
// new value through external means.

struct validation_error_handler_data
{
    owned_id id;
    bool error_occurred;
    string error_message;
    validation_error_report report;

    validation_error_handler_data() : error_occurred(false) {}
};

// Manually clear the error message stored in the given validation data.
static inline void
clear_error(validation_error_handler_data& data)
{
    data.error_occurred = false;
}

template<class Wrapped>
struct validation_error_handler
  : accessor<typename accessor_value_type<Wrapped>::type>
{
    typedef typename accessor_value_type<Wrapped>::type wrapped_value_type;
    validation_error_handler(
        Wrapped wrapped, validation_error_handler_data* data)
      : wrapped_(wrapped), data_(data)
    {}
    bool is_gettable() const { return wrapped_.is_gettable(); }
    wrapped_value_type const& get() const { return wrapped_.get(); }
    alia__shared_ptr<wrapped_value_type> get_ptr() const
    { return wrapped_.get_ptr(); }
    id_interface const& id() const { return wrapped_.id(); }
    bool is_settable() const { return wrapped_.is_settable(); }
    void set(wrapped_value_type const& value) const
    {
        try
        {
            wrapped_.set(value);
            clear_error(*data_);
        }
        catch (validation_error& e)
        {
            data_->error_message = e.what();
            data_->error_occurred = true;
        }
    }
 private:
    Wrapped wrapped_;
    validation_error_handler_data* data_;
};

template<class Accessor>
validation_error_handler<Accessor>
make_validation_error_handler(
    dataless_ui_context& ctx, Accessor const& accessor,
    validation_error_handler_data& data)
{
    if (is_refresh_pass(ctx))
    {
        if (!data.id.matches(accessor.id()))
        {
            clear_error(data);
            data.id.store(accessor.id());
        }
        if (data.error_occurred)
        {
            data.report.message = &data.error_message;
            add_validation_error_report(ctx, &data.report);
        }
    }
    return validation_error_handler<Accessor>(accessor, &data);
}

}

#endif
