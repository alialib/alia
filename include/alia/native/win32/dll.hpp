#ifndef ALIA_NATIVE_WIN32_DLL_HPP
#define ALIA_NATIVE_WIN32_DLL_HPP

#include <alia/exception.hpp>
#include <alia/native/win32/windows.hpp>
#include <string>

namespace alia { namespace native {

class dll
{
 public:
    dll(char const* file)
    {
        name = file;
        lib = LoadLibrary(file);
        if (lib == NULL)
            throw exception("unable to load DLL: " + name);
    }
    ~dll() { FreeLibrary(lib); }

    template<class Function>
    void load_function(Function*& f, char const* name)
    {
        f = (Function*)GetProcAddress(lib, name);
        if (f == NULL)
        {
            throw exception("unable to load DLL function: " + this->name +
                ": "     + name);
        }
    }

 private:
    HMODULE lib;
    std::string name;
};

}}

#endif
