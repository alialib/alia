:: Install additional Conan remotes that are needed to build alia.
@echo off
echo "Setting up Conan..."
conan remote add bincrafters https://api.bintray.com/conan/bincrafters/public-conan || exit /b
