#ifndef ALIA_MILLISECOND_CLOCK_HPP
#define ALIA_MILLISECOND_CLOCK_HPP

#include <boost/date_time/posix_time/posix_time.hpp>

namespace alia {

class millisecond_clock
{
 public:
    millisecond_clock()
      : start_time_(boost::posix_time::microsec_clock::local_time())
    {}
    unsigned get_tick_count() const
    {
        return unsigned(
            (boost::posix_time::microsec_clock::local_time()
            - start_time_).total_milliseconds());
    }
 private:
    boost::posix_time::ptime start_time_;
};

}

#endif
