@echo off
setlocal enabledelayedexpansion

:: -----------------------------------------------------------------------------
:: -----------------------------------------------------------------------------
:: 0. SELECT ACTION
:: -----------------------------------------------------------------------------
:SELECT_ACTION
cls
echo ==========================================
echo           GAME ENGINE LAUNCHER
echo ==========================================
echo  1. Full Rebuild (Clean + Build + Run)
echo  2. Run Existing Game
echo ==========================================
set "action_choice="
set /p action_choice="Enter number (Default: 1): "

if "%action_choice%"=="" set action_choice=1

:: Validate number
echo %action_choice%| findstr /r "^[1-2]$" >nul
if errorlevel 1 goto RETRY_ACTION

if "%action_choice%"=="2" goto SELECT_BUILD_TYPE_FOR_RUN
goto SELECT_COMPILER

:RETRY_ACTION
echo [ERROR] Invalid selection!
pause
goto SELECT_ACTION

:: -----------------------------------------------------------------------------
:: 1. CHOOSE COMPILER / GENERATOR
:: -----------------------------------------------------------------------------
:SELECT_COMPILER
cls
echo ==========================================
echo       SELECT COMPILER / GENERATOR
echo ==========================================
echo  1. Visual Studio 6 (2000-ish) [Legacy]
echo  2. Visual Studio 2005
echo  3. Visual Studio 2008
echo  4. Visual Studio 2010
echo  5. Visual Studio 2012
echo  6. Visual Studio 2013
echo  7. Visual Studio 2015
echo  8. Visual Studio 2017
echo  9. Visual Studio 2019
echo 10. Visual Studio 2022
echo 11. Visual Studio 2026 (Default - Will try VS 2022 or Latest)
echo 12. MinGW (MinGW Makefiles)
echo 13. Clang (Ninja / NMake)
echo ==========================================
set "comp_choice="
set /p comp_choice="Enter number (Default: 11): "

if "%comp_choice%"=="" set comp_choice=11

:: Validate number
echo %comp_choice%| findstr /r "^[0-9]*$" >nul
if errorlevel 1 goto RETRY_COMPILER

if %comp_choice% LSS 1 goto RETRY_COMPILER
if %comp_choice% GTR 13 goto RETRY_COMPILER

goto COMPILER_CHOSEN

:RETRY_COMPILER
echo.
echo [ERROR] Invalid selection! Please enter a number between 1 and 13.
pause
goto SELECT_COMPILER

:COMPILER_CHOSEN
set GENERATOR=""

if "%comp_choice%"=="1" set GENERATOR="Visual Studio 6"
if "%comp_choice%"=="2" set GENERATOR="Visual Studio 8 2005"
if "%comp_choice%"=="3" set GENERATOR="Visual Studio 9 2008"
if "%comp_choice%"=="4" set GENERATOR="Visual Studio 10 2010"
if "%comp_choice%"=="5" set GENERATOR="Visual Studio 11 2012"
if "%comp_choice%"=="6" set GENERATOR="Visual Studio 12 2013"
if "%comp_choice%"=="7" set GENERATOR="Visual Studio 14 2015"
if "%comp_choice%"=="8" set GENERATOR="Visual Studio 15 2017"
if "%comp_choice%"=="9" set GENERATOR="Visual Studio 16 2019"
if "%comp_choice%"=="10" set GENERATOR="Visual Studio 17 2022"
if "%comp_choice%"=="11" set "GENERATOR=" 
if "%comp_choice%"=="12" set GENERATOR="MinGW Makefiles"
if "%comp_choice%"=="13" set GENERATOR="Ninja"

echo Selected Compiler Option: %comp_choice%

:: -----------------------------------------------------------------------------
:: 2. CHOOSE BUILD TYPE (FOR REBUILD)
:: -----------------------------------------------------------------------------
:SELECT_BUILD_TYPE
echo.
echo ==========================================
echo           SELECT BUILD TYPE
echo ==========================================
echo  1. Debug
echo  2. Release (Default)
echo  3. RelWithDebInfo
echo  4. MinSizeRel
echo ==========================================
set "type_choice="
set /p type_choice="Enter number (Default: 2): "

if "%type_choice%"=="" set type_choice=2

:: Validate number
echo %type_choice%| findstr /r "^[0-9]*$" >nul
if errorlevel 1 goto RETRY_BUILD_TYPE

if %type_choice% LSS 1 goto RETRY_BUILD_TYPE
if %type_choice% GTR 4 goto RETRY_BUILD_TYPE

goto BUILD_TYPE_CHOSEN

:RETRY_BUILD_TYPE
echo.
echo [ERROR] Invalid selection! Please enter a number between 1 and 4.
pause
goto SELECT_BUILD_TYPE

:BUILD_TYPE_CHOSEN
set BUILD_TYPE=Release
if "%type_choice%"=="1" set BUILD_TYPE=Debug
if "%type_choice%"=="2" set BUILD_TYPE=Release
if "%type_choice%"=="3" set BUILD_TYPE=RelWithDebInfo
if "%type_choice%"=="4" set BUILD_TYPE=MinSizeRel

echo Selected Build Type: %BUILD_TYPE%

:: -----------------------------------------------------------------------------
:: 2c. CONFIRM CONFIG
:: -----------------------------------------------------------------------------
:CONFIRM_CONFIG
cls
echo.
echo ==========================================
echo           CONFIRM CONFIGURATION
echo ==========================================
if not defined GENERATOR (
    echo  Generator:  Default (Auto-Detect VS^)
) else (
    echo  Generator:  %GENERATOR%
)
echo  Build Type: %BUILD_TYPE%
echo ==========================================
set "confirm="
set /p confirm="Do you want to proceed? (Y/N, M=Main Menu) [Default: Y]: "
if "%confirm%"=="" set confirm=y

if /i "%confirm%"=="m" goto SELECT_ACTION
if /i "%confirm%"=="n" goto SELECT_COMPILER
if /i "%confirm%"=="y" goto CLEAN_FOLDERS
goto CONFIRM_CONFIG


:SELECT_BUILD_TYPE_FOR_RUN
echo.
echo ==========================================
echo      SELECT VERSION TO RUN
echo ==========================================
echo ==========================================
echo  1. Debug
echo  2. Release (Default)
echo  M. Back to Main Menu
echo ==========================================
set "run_type_choice="
set /p run_type_choice="Enter selection (Default: 2): "

if "%run_type_choice%"=="" set run_type_choice=2

if /i "%run_type_choice%"=="m" goto SELECT_ACTION

:: Validate number
echo %run_type_choice%| findstr /r "^[1-2]$" >nul
if errorlevel 1 goto RETRY_RUN_TYPE

set BUILD_TYPE=Release
if "%run_type_choice%"=="1" set BUILD_TYPE=Debug
if "%run_type_choice%"=="2" set BUILD_TYPE=Release

goto RUN_GAME

:RETRY_RUN_TYPE
echo [ERROR] Invalid selection!
pause
goto SELECT_BUILD_TYPE_FOR_RUN

:: -----------------------------------------------------------------------------
:: 3. CLEAN OLD FILES
:: -----------------------------------------------------------------------------
:CLEAN_FOLDERS
echo.
echo ==========================================
echo        CLEANING BIN AND BUILD...
echo ==========================================

:: Kill the game process if running
taskkill /F /IM GameEngine.exe >nul 2>&1

:: Kill lingering build processes (fix file lock issues if script was interrupted)
taskkill /F /IM cmake.exe >nul 2>&1
taskkill /F /IM MSBuild.exe >nul 2>&1
taskkill /F /IM cl.exe >nul 2>&1
taskkill /F /IM link.exe >nul 2>&1
taskkill /F /IM ninja.exe >nul 2>&1

timeout /t 1 /nobreak >nul

if exist "build" (
    echo Deleting build folder...
    rmdir /s /q "build"
    if exist "build" (
        echo [WARNING] Could not delete 'build' folder. Use check file locks.
    )
) else (
    echo Build folder not found.
)

if exist "bin" (
    echo Deleting bin folder...
    rmdir /s /q "bin"
    if exist "bin" (
        echo [WARNING] Could not delete 'bin' folder. Use check file locks.
    )
) else (
    echo Bin folder not found.
)

:: -----------------------------------------------------------------------------
:: 4. RUN CMAKE
:: -----------------------------------------------------------------------------
echo.
echo ==========================================
echo      CONFIGURING AND BUILDING...
echo ==========================================
echo Generator: %GENERATOR%
echo Build Type: %BUILD_TYPE%

:: Check for CMake in PATH
where cmake >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set CMAKE_CMD=cmake
) else (
    echo CMake not found in PATH. Searching common locations...
    
    :: Try VS 2022 / VS 18 (Future) Preview Path
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE_CMD=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    ) else (
         if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
            set "CMAKE_CMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
         ) else (
             echo [ERROR] CMake not found! Please install CMake or add it to PATH.
             pause
             exit /b 1
         )
    )
)

echo Using CMake: "!CMAKE_CMD!"

if not defined GENERATOR (
    "!CMAKE_CMD!" -B build
) else (
    "!CMAKE_CMD!" -G %GENERATOR% -B build
)

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake Configuration failed!
    pause
    exit /b %ERRORLEVEL%
)

:: CMake will pick the correct default target (ALL_BUILD for VS, all for Makefiles)
"!CMAKE_CMD!" --build build --config %BUILD_TYPE%

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    pause
    exit /b %ERRORLEVEL%
)

:: -----------------------------------------------------------------------------
:: 5. RUN EXECUTABLE
:: -----------------------------------------------------------------------------
:RUN_GAME
echo.
echo ==========================================
echo          RUNNING GAME ENGINE
echo ==========================================

set EXE_PATH=bin\%BUILD_TYPE%\GameEngine.exe

:: CMake with VS generators puts output in bin/Debug or bin/Release
if exist "bin\%BUILD_TYPE%\GameEngine.exe" (
    set EXE_PATH=bin\%BUILD_TYPE%\GameEngine.exe
) else (
    :: Some configs might put it directly in bin if not using multi-config generator
    if exist "bin\GameEngine.exe" (
        set EXE_PATH=bin\GameEngine.exe
    )
)

if exist "%EXE_PATH%" (
    echo Launching: %EXE_PATH%
    "%EXE_PATH%"
) else (
    echo [ERROR] Executable not found at %EXE_PATH%
    pause
)

pause
