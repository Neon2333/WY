@echo off

if exist build rmdir /s /q build
if exist build_Debug rmdir /s /q build_Debug
if exist build_Release rmdir /s /q build_Release 

echo clean complete!
