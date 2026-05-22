@echo off
:: build.bat - Windows 版本
cmake -B build_Release -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/5.15.2/msvc2019_64"
cmake --build build_Release --config Release
cd ..
echo Build complete!