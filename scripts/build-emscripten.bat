@echo off
setlocal EnableDelayedExpansion

:: Usage: build-emscripten.bat [target]
:: Default target: alia_app

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=alia_app"

set "EMSDK_ROOT=c:\dev\emsdk"
if not "%EMSDK%"=="" set "EMSDK_ROOT=%EMSDK%"

if not exist "%EMSDK_ROOT%\emsdk_env.bat" (
    echo Error: EMSDK not found at %EMSDK_ROOT%
    exit /b 1
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo Error: cmake.exe not found in PATH.
    exit /b 1
)

set "BUILD_DIR=build\Emscripten"
set "HOST_BUILDER=build\Release\tools\alia_asset_builder.exe"
if not exist "%HOST_BUILDER%" (
    echo Error: Native asset builder not found at %HOST_BUILDER%
    echo Build the desktop tree first: scripts\build.bat alia_app
    exit /b 1
)

call "%EMSDK_ROOT%\emsdk_env.bat"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

if not exist "%BUILD_DIR%\build.ninja" (
    echo Configuring Emscripten build...
    emcmake cmake -G Ninja -B "%BUILD_DIR%" ^
        -DCMAKE_BUILD_TYPE=Release ^
        -DALIA_ENABLE_EXAMPLES=ON ^
        -DALIA_ENABLE_TESTING=OFF ^
        -DALIA_ENABLE_BENCHMARKS=OFF
    if errorlevel 1 exit /b 1
)

cmake --build "%BUILD_DIR%" --target %TARGET%
exit /b %errorlevel%
