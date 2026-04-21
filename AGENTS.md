# Build Environment and Execution

A build directory is already set up in `build/Release`. To build it, use
`scripts/build.bat` from a normal Windows command prompt. It will take care of
setting up the Visual Studio environment and invoking CMake. It takes a target
as its own argument. Right now most development is done around the `alia_app`
demo/sandbox target.
