#ifndef ALIA_SINGLETON_HPP
#define ALIA_SINGLETON_HPP

#include <boost/noncopyable.hpp>

namespace alia {

template<class Derived>
class singleton : boost::noncopyable
{
 public:
    static Derived& get_instance()
    {
        static Derived instance;
        return instance;
    }
};

}

#endif
