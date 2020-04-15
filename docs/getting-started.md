Getting Started
===============

The Core Library
----------------

The core of alia should work on any C++14 compiler. It has no external
dependencies. It's regularly tested on:

- Visual C++ 2015 and 2017 (x86 and x64 for both, C++14 features only)
- GCC 5 (C++14 only) and 7 (with C++17 features)
- Clang 4.0 (C++14 only) and 5.0 (with C++17 features)
- Emscripten 1.39.7 (including C++17 features)

It should support more recent versions of the above. If you find otherwise,
please open an issue.

### Getting It

alia is distributed as a single header (`alia.hpp`) which is attached as an
asset to every release in GitHub. You can download the latest release here:

https://github.com/tmadden/alia/releases/latest/download/alia.hpp

If you want to play around with the latest version on the `master` branch, that
header is available via GitHub Pages:

https://tmadden.github.io/alia/alia.hpp

### Using It

alia follows the normal conventions of a header-only library that also includes
implementation (.cpp) code:

*In one and only one of your source files, you should `#define
ALIA_IMPLEMENTAITON` before including `alia.hpp`.*

In your other source files, you can simply include it directly.

There are also some [config options](configuration.md) that you may want to set.

Experimental Wrappers
---------------------

If you want to play around with alia, there are experimental adaptors for
asm-dom and Qt:

https://github.com/tmadden/alia/tree/master/examples

They're both very limited and aren't meant for production use, but they're fine
for playing around with alia. Both have small sandbox projects and README files
with instructions on building them.
