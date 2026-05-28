@echo off
:: build.bat - Windows 版本
cmake -B build_Release -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="D:\Qt\6.8.1\msvc2022_64"
cmake --build build_Release --config Release
cd ..
echo Build complete!