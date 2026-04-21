:: This script can be used to build the project after it has already been
:: configured with CMake (for Visual Studio, x64 architecture). It's designed
:: to be resilient against environmental issues and should be able to run from
:: a normal Windows command prompt. It uses the CMake cache to determine the
:: path to the Visual Studio installation that was used to configure the build
:: and then calls vcvars64.bat to set up the environment. It also attempts to
:: find CMake if it's not already in the path.

:: Usage: build.bat <target>

@echo off
setlocal EnableDelayedExpansion

:: LOCATE VISUAL STUDIO INSTALLATION

:: Extract the compiler path from the CMake cache
set "CACHE_FILE=build\Release\CMakeCache.txt"
for /f "tokens=2 delims==" %%I in ('findstr /I /C:"CMAKE_CXX_COMPILER:FILEPATH=" "%CACHE_FILE%" 2^>nul') do set "CL_PATH=%%I"

if "%CL_PATH%"=="" (
    echo Error: Could not find CMAKE_CXX_COMPILER in %CACHE_FILE%
    exit /b 1
)

:: Normalize forward slashes to backslashes
set "CL_PATH=%CL_PATH:/=\%"

:: Split the path at \VC\Tools\ to isolate the Visual Studio installation root
for /f "tokens=1 delims=|" %%A in ("%CL_PATH:\VC\Tools\=|%") do set "VS_ROOT=%%A"

set "VCVARS=%VS_ROOT%\VC\Auxiliary\Build\vcvars64.bat"

if not exist "%VCVARS%" (
    echo Error: Could not resolve vcvars64.bat from cache path: %VCVARS%
    exit /b 1
)

:: LOCATE CMAKE

set "CMAKE_CMD="

:: Try A: System PATH
where cmake >nul 2>nul
if %errorlevel% equ 0 set "CMAKE_CMD=cmake"

:: Try B: Standard Standalone Installation
if "!CMAKE_CMD!"=="" if exist "C:\Program Files\CMake\bin\cmake.exe" set "CMAKE_CMD=C:\Program Files\CMake\bin\cmake.exe"

:: Try C: Visual Studio Bundled CMake
if "!CMAKE_CMD!"=="" if exist "%VS_ROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" set "CMAKE_CMD=%InstallDir%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"

if "!CMAKE_CMD!"=="" (
    echo Error: Could not find cmake.exe in PATH, Program Files, or Visual Studio.
    exit /b 1
)

:: BUILD
call "%VCVARS%"
cmake --build build/Release --target %1
