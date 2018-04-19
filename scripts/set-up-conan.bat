:: Install additional Conan remotes that are needed to build alia.
@echo off
echo "Setting up Conan..."
conan remote add conan-community https://api.bintray.com/conan/conan-community/conan || exit /b
conan remote add conan-transit https://api.bintray.com/conan/conan/conan-transit || exit /b
