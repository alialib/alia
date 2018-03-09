:: Apply our .clang-format styling to all C++ source files.
:: Note that this should be run from the root directory.
@echo off
setlocal enabledelayedexpansion
for /F usebackq^ tokens^=*^ delims^=^ eol^= %%f in (`dir /s /b *.hpp *.cpp *.ipp`) do (
    clang-format -style=file -i %%f
)
endlocal
