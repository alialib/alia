#ifndef ALIA_EXCEPTION_HPP
#define ALIA_EXCEPTION_HPP

#include <exception>
#include <string>
#include <boost/shared_ptr.hpp>

namespace alia {

class exception : public std::exception
{
 public:
    exception(std::string const& msg)
      : msg_(new std::string(msg))
    {}
    ~exception() throw() {}

    virtual char const* what() const throw()
    { return msg_->c_str(); }

    // Add another level of context to the error messsage.
    void add_context(std::string const& str)
    { *msg_ = str + "\n" + *msg_; }

 private:
    boost::shared_ptr<std::string> msg_;
};

}

#endif
