#ifndef ALIA_MILLISECOND_CLOCK_HPP
#define ALIA_MILLISECOND_CLOCK_HPP

#include <boost/date_time/posix_time/posix_time.hpp>
#include <alia/singleton.hpp>

namespace alia {

class millisecond_clock : public singleton<millisecond_clock>
{
 public:
    unsigned get_tick_count() const
    {
        return unsigned(
            (boost::posix_time::microsec_clock::local_time()
            - start_time_).total_milliseconds());
    }
 private:
    millisecond_clock()
      : start_time_(boost::posix_time::microsec_clock::local_time())
    {}
    friend class singleton<millisecond_clock>;
    boost::posix_time::ptime start_time_;
};

}

#endif
