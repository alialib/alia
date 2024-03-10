#!/bin/bash
# Configure a CMake build for the web UI.
emcmake cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -D"VCPKG_CHAINLOAD_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" -DVCPKG_TARGET_TRIPLET=wasm32-emscripten -D"CMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DALIA_ENABLE_UI=ON -DALIA_ENABLE_SDL=ON -DALIA_ENABLE_GLFW=OFF -DALIA_BUILD_EXAMPLES=ON -DALIA_ENABLE_HTML=OFF -DALIA_BUILD_DOCS=OFF ..
