Experimental Qt Wrapper
=======================

This is an experimental wrapper for Qt. It's intended as a proof of concept, an
illustration of how you might create an actual wrapper, and to allow you to play
around with alia. **This is not intended for real use.**

If you're interested in seeing the actual wrapper code, check out the files
`adaptor.hpp` and `adaptor.cpp`.

Building
--------

1. Clone this repository and change to this directory:
   ```shell
   git clone https://github.com/tmadden/alia
   cd alia/examples/qt
   ```

2. Get a copy of `alia.hpp`:
   ```shell
   wget https://alia.dev/alia.hpp
   ```

3. Create a build directory:

   ```shell
   mkdir build
   cd build
   ```

4. Either install Qt directly or use Conan to install it:

   ```shell
   conan install ..
   ```

5. Build the project:
   ```shell
   cmake ..
   cmake --build .
   ```

   (Or on Windows, you might need to explicitly specify a generator...)

   ```shell
   cmake -G"NMake Makefiles" ..
   cmake --build .
   ```

The Sandbox
-----------

This produces a ``sandbox`` executable. You can edit `sandbox.cpp` to play
around with the contents of the sandbox.
