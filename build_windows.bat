@echo off
REM =============================================
REM  Input Stream - Windows Build Script
REM  By Rhythm113
REM =============================================

echo.
echo  Building Input Stream for Windows...
echo  =====================================
echo.

REM Check for cmake
where cmake >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo  [!] CMake not found. Please install CMake 3.15+ and add to PATH.
    exit /b 1
)

REM Configure
echo  [1/2] Configuring...
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo  [!] Configuration failed.
    exit /b 1
)

REM Build
echo  [2/2] Building...
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo  [!] Build failed.
    exit /b 1
)

echo.
echo  =============================================
echo  [+] Build successful!
echo  [+] Binary: build\input_stream.exe
echo      (or build\Release\input_stream.exe for MSVC)
echo  =============================================
echo.
