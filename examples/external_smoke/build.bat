@echo off
setlocal EnableDelayedExpansion

:: Configure (once) and build the external-nest smoke from this directory.
:: Usage: build.bat
:: Optional: set VCPKG_ROOT or CMAKE_TOOLCHAIN_FILE before running.

cd /d "%~dp0"

if "%CMAKE_TOOLCHAIN_FILE%"=="" (
    if defined VCPKG_ROOT (
        set "CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
    ) else if exist "C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake" (
        set "CMAKE_TOOLCHAIN_FILE=C:\dev\vcpkg\scripts\buildsystems\vcpkg.cmake"
    )
)

if "%CMAKE_TOOLCHAIN_FILE%"=="" (
    echo Error: Set CMAKE_TOOLCHAIN_FILE or VCPKG_ROOT to your vcpkg toolchain.
    exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo Error: cmake.exe not found in PATH.
    exit /b 1
)

:: Prefer vcvars from an existing Alia desktop build cache when present so
:: configure picks MSVC (matching the main tree) instead of a stray clang.
set "ALIA_ROOT=%~dp0..\.."
set "CACHE_FILE=%ALIA_ROOT%\build\Release\CMakeCache.txt"
set "VCVARS="
for /f "tokens=2 delims==" %%I in ('findstr /I /C:"CMAKE_CXX_COMPILER:FILEPATH=" "%CACHE_FILE%" 2^>nul') do set "CL_PATH=%%I"
if not "%CL_PATH%"=="" (
    set "CL_PATH=%CL_PATH:/=\%"
    for /f "tokens=1 delims=|" %%A in ("!CL_PATH:\VC\Tools\=|!") do set "VS_ROOT=%%A"
    if exist "!VS_ROOT!\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=!VS_ROOT!\VC\Auxiliary\Build\vcvars64.bat"
)

if not "%VCVARS%"=="" (
    call "%VCVARS%"
) else (
    echo Warning: could not locate vcvars64.bat; building with current environment.
)

set "BUILD_DIR=build"

if not exist "%BUILD_DIR%\build.ninja" if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo Configuring external smoke...
    cmake -G Ninja -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release ^
        -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%"
    if errorlevel 1 exit /b 1
)

cmake --build "%BUILD_DIR%" --target alia_external_smoke
exit /b %errorlevel%
