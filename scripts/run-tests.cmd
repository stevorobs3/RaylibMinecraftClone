@echo off
setlocal
set "BUILD_DIR=%~1"
if "%BUILD_DIR%"=="" set "BUILD_DIR=build\sublime-Debug"
cmake --build "%BUILD_DIR%" --target minecraft_tests --parallel 6
if errorlevel 1 exit /b 1
ctest --test-dir "%BUILD_DIR%" --output-on-failure
exit /b %errorlevel%
