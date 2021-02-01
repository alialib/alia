Getting Started
===============

The Core Library
----------------

The core of alia should work on any C++14 compiler. It has no external
dependencies. The following compilers are officially supported:

- Visual C++ 2015 and higher
- GCC 5 and higher
- Clang 4.0 and higher
- Emscripten 1.39.7 and higher

### Getting It

alia is distributed as a single header (`alia.hpp`) which is attached as an
asset to every release in GitHub. You can download the latest release here:

https://github.com/tmadden/alia/releases/latest/download/alia.hpp

If you want to play around with the latest version on the `master` branch, that
header is available via GitHub Pages:

https://alia.dev/alia.hpp

### Using It

alia follows the normal conventions of a header-only library that also includes
implementation (.cpp) code:

*In one and only one of your source files, you should `#define
ALIA_IMPLEMENTAITON` before including `alia.hpp`.*

In your other source files, you can simply include it directly.

There are also some [config options](configuration.md) that you may want to set.
