#ifndef ALIA_STATE_IO_HPP
#define ALIA_STATE_IO_HPP

#include <alia/forward.hpp>
#include <alia/typedefs.hpp>
#include <string>

namespace alia {

class state_writer
{
 public:
    virtual ~state_writer() {}
    virtual void write_string(std::string const& s) = 0;
    virtual void write_bool(bool b) = 0;
    virtual void write_int32(int32 i) = 0;
    virtual void write_float(float f) = 0;
    virtual void write_double(double d) = 0;
};

void save_state(context& ctx, state_writer& writer);

class state_reader
{
 public:
    virtual ~state_reader() {}
    virtual std::string read_string() = 0;
    virtual bool read_bool() = 0;
    virtual int32 read_int32() = 0;
    virtual float read_float() = 0;
    virtual double read_double() = 0;
};

void restore_state(context& ctx, state_reader& reader);

}

#endif
