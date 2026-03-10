@echo off
setlocal enabledelayedexpansion

set "LOCK_FILE=%~dp0build_engine.lock"

if "%~1"==":LOCKED_SESSION" goto :MAIN_LOGIC

(
    "%COMSPEC%" /c "%~f0" :LOCKED_SESSION
) 9>"%LOCK_FILE%" || (
    echo.
    echo [ERROR] Another instance of 'Game Engine Builder' is already running!
    echo         (File '%LOCK_FILE%' is locked)
    echo.
    pause
    exit /b
)

exit /b

:MAIN_LOGIC
title Axis Engine Builder

:SELECT_ACTION
cls
echo ==========================================
echo           GAME ENGINE LAUNCHER
echo ==========================================
echo  1. Full Rebuild (Clean + Build + Run)
echo  2. Quick Build (Build + Run - FAST)
echo  3. Run Existing Game
echo ==========================================
set "action_choice="
set /p action_choice="Enter number (Default: 1): "

if "%action_choice%"=="" set action_choice=1

echo %action_choice%| findstr /r "^[1-3]$" >nul
if errorlevel 1 goto RETRY_ACTION

if "%action_choice%"=="1" (
    set "QUICK_BUILD="
    goto SELECT_COMPILER
)
if "%action_choice%"=="2" (
    set "QUICK_BUILD=1"
    goto SELECT_BUILD_TYPE
)
if "%action_choice%"=="3" goto SELECT_BUILD_TYPE_FOR_RUN

:RETRY_ACTION
echo [ERROR] Invalid selection!
pause
goto SELECT_ACTION

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
echo 11. Visual Studio 2026
echo 12. MinGW (MinGW Makefiles)
echo 13. Clang (Ninja / NMake)
echo 14. Auto-Detect (Default - CMake will pick latest VS)
echo ------------------------------------------
echo  B. Back to Main Menu
echo ==========================================
set "comp_choice="
set /p comp_choice="Enter number (Default: 14): "

if "%comp_choice%"=="" set comp_choice=14
if /i "%comp_choice%"=="b" goto SELECT_ACTION

echo %comp_choice%| findstr /r "^[0-9]*$" >nul
if errorlevel 1 goto RETRY_COMPILER

if %comp_choice% LSS 1 goto RETRY_COMPILER
if %comp_choice% GTR 14 goto RETRY_COMPILER

goto COMPILER_CHOSEN

:RETRY_COMPILER
echo.
echo [ERROR] Invalid selection! Please enter a number between 1 and 14.
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
if "%comp_choice%"=="11" set GENERATOR="Visual Studio 18 2026" 
if "%comp_choice%"=="12" set GENERATOR="MinGW Makefiles"
if "%comp_choice%"=="13" set GENERATOR="Ninja"
if "%comp_choice%"=="14" set "GENERATOR="

echo Selected Compiler Option: %comp_choice%

:SELECT_BUILD_TYPE
cls
echo.
echo ==========================================
echo           SELECT BUILD TYPE
echo ==========================================
echo  1. Debug
echo  2. Release (Default)
echo  3. RelWithDebInfo
echo  4. MinSizeRel
echo ------------------------------------------
echo  B. Back
echo ==========================================
set "type_choice="
set /p type_choice="Enter number (Default: 2): "

if "%type_choice%"=="" set type_choice=2
if /i "%type_choice%"=="b" (
    if defined QUICK_BUILD goto SELECT_ACTION
    goto SELECT_COMPILER
)

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

if defined QUICK_BUILD goto CONFIRM_CONFIG
goto SELECT_CLEAN_MODE

:SELECT_CLEAN_MODE
cls
echo.
echo ==========================================
echo           SELECT CLEAN MODE
echo ==========================================
echo  1. Strict (Fail if cannot clean build/bin)
echo  2. Soft   (Warning only if cannot clean) [Default]
echo ------------------------------------------
echo  B. Back
echo ==========================================
set "clean_mode_choice="
set /p clean_mode_choice="Enter number (Default: 2): "

if "%clean_mode_choice%"=="" set clean_mode_choice=2
if /i "%clean_mode_choice%"=="b" goto SELECT_BUILD_TYPE

echo %clean_mode_choice%| findstr /r "^[1-2]$" >nul
if errorlevel 1 goto RETRY_CLEAN_MODE

set CLEAN_MODE=Soft
if "%clean_mode_choice%"=="1" set CLEAN_MODE=Strict
if "%clean_mode_choice%"=="2" set CLEAN_MODE=Soft

echo Selected Clean Mode: %CLEAN_MODE%
goto SELECT_DEBUG_SYSTEM

:RETRY_CLEAN_MODE
echo [ERROR] Invalid selection!
pause
goto SELECT_CLEAN_MODE

:SELECT_DEBUG_SYSTEM
cls
echo.
echo ==========================================
echo           ENABLE DEBUG SYSTEM?
echo ==========================================
echo  1. No (Default)
echo  2. Yes (Enables Debug Tools)
echo ------------------------------------------
echo  B. Back
echo ==========================================
set "debug_sys_choice="
set /p debug_sys_choice="Enter number (Default: 1): "

if "%debug_sys_choice%"=="" set debug_sys_choice=1
if /i "%debug_sys_choice%"=="b" goto SELECT_CLEAN_MODE

echo %debug_sys_choice%| findstr /r "^[1-2]$" >nul
if errorlevel 1 goto RETRY_DEBUG_SYSTEM

set ENABLE_DEBUG_SYSTEM=OFF
if "%debug_sys_choice%"=="2" set ENABLE_DEBUG_SYSTEM=ON

goto CONFIRM_CONFIG

:RETRY_DEBUG_SYSTEM
echo [ERROR] Invalid selection!
pause
goto SELECT_DEBUG_SYSTEM


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
if defined QUICK_BUILD (
    echo  Build Mode: QUICK
) else (
    echo  Clean Mode: %CLEAN_MODE%
    echo  Debug Sys:  %ENABLE_DEBUG_SYSTEM%
)
echo ==========================================
set "confirm="
set /p confirm="Do you want to proceed? (Y/N, B=Back) [Default: Y]: "
if "%confirm%"=="" set confirm=y

if /i "%confirm%"=="b" (
    if defined QUICK_BUILD goto SELECT_BUILD_TYPE
    goto SELECT_DEBUG_SYSTEM
)
if /i "%confirm%"=="n" goto SELECT_ACTION
if /i "%confirm%"=="y" (
    cls
    goto CLEAN_FOLDERS
)
goto CONFIRM_CONFIG

:SELECT_BUILD_TYPE_FOR_RUN
cls
echo.
echo ==========================================
echo      SELECT VERSION TO RUN
echo ==========================================
echo  1. Debug
echo  2. Release (Default)
echo ------------------------------------------
echo  B. Back to Main Menu
echo ==========================================
set "run_type_choice="
set /p run_type_choice="Enter selection (Default: 2): "

if "%run_type_choice%"=="" set run_type_choice=2
if /i "%run_type_choice%"=="b" goto SELECT_ACTION

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

:CLEAN_FOLDERS
echo.
echo ==========================================
echo        CLEANING BIN AND BUILD...
echo ==========================================

taskkill /F /IM AxisEngine.exe >nul 2>&1
taskkill /F /IM cmake.exe >nul 2>&1
taskkill /F /IM MSBuild.exe >nul 2>&1
taskkill /F /IM cl.exe >nul 2>&1
taskkill /F /IM link.exe >nul 2>&1
taskkill /F /IM ninja.exe >nul 2>&1

timeout /t 1 /nobreak >nul

if defined QUICK_BUILD (
    echo [SKIP] Quick Build enabled. Skipping folder deletion...
    goto SKIP_CLEAN
)

if exist "build" (
    echo Deleting build folder...
    rmdir /s /q "build"
    if exist "build" (
        echo [FAILED] Could not delete 'build' folder.
        if "%CLEAN_MODE%"=="Strict" (
            echo [ERROR] Strict Mode enabled. Aborting because cleanup failed.
            pause
            goto EXIT_SCRIPT
        )
    ) else (
        echo [DELETED] 'build' folder.
    )
)

if exist "bin\%BUILD_TYPE%" (
    echo Deleting bin\%BUILD_TYPE% folder...
    rmdir /s /q "bin\%BUILD_TYPE%"
    if exist "bin\%BUILD_TYPE%" (
        echo [FAILED] Could not delete 'bin\%BUILD_TYPE%' folder.
        if "%CLEAN_MODE%"=="Strict" (
            echo [ERROR] Strict Mode enabled. Aborting because cleanup failed.
            pause
            goto EXIT_SCRIPT
        )
    ) else (
        echo [DELETED] 'bin\%BUILD_TYPE%' folder.
    )
) else (
    if exist "bin\AxisEngine.exe" (
        del /f /q "bin\AxisEngine.exe"
    )
)

:SKIP_CLEAN

title Axis Engine Builder - Building...
echo.
echo ==========================================
echo      CONFIGURING AND BUILDING...
echo ==========================================

where cmake >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set CMAKE_CMD=cmake
) else (
    if exist "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
        set "CMAKE_CMD=C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    ) else (
         if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" (
            set "CMAKE_CMD=C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
         ) else (
             echo [ERROR] CMake not found!
             pause
             exit /b 1
         )
    )
)

if not defined QUICK_BUILD (
    if not defined GENERATOR (
        "!CMAKE_CMD!" -B build -DENABLE_DEBUG_SYSTEM=!ENABLE_DEBUG_SYSTEM!
    ) else (
        "!CMAKE_CMD!" -G %GENERATOR% -B build -DENABLE_DEBUG_SYSTEM=!ENABLE_DEBUG_SYSTEM!
    )
) else (
    if not exist "build" (
        "!CMAKE_CMD!" -B build
    )
)

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] CMake Configuration failed!
    pause
    goto EXIT_SCRIPT
)

"!CMAKE_CMD!" --build build --config %BUILD_TYPE%

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    goto EXIT_SCRIPT
)

:RUN_GAME
title Axis Engine Builder - Running...
echo.
echo ==========================================
echo          RUNNING GAME ENGINE
echo ==========================================

set EXE_PATH=bin\%BUILD_TYPE%\AxisEngine.exe

if exist "bin\%BUILD_TYPE%\AxisEngine.exe" (
    set EXE_PATH=bin\%BUILD_TYPE%\AxisEngine.exe
) else (
    if exist "bin\AxisEngine.exe" (
        set EXE_PATH=bin\AxisEngine.exe
    )
)

if exist "%EXE_PATH%" (
    "%EXE_PATH%"
    if !ERRORLEVEL! NEQ 0 (
        echo.
        echo [ERROR] Game exited with error code: !ERRORLEVEL!
    )
) else (
    echo [ERROR] Executable not found!
)

:EXIT_SCRIPT
exit /b %ERRORLEVEL%
