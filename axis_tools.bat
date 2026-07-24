@echo off
setlocal enabledelayedexpansion

:MAIN_LOGIC
title Axis Engine Tools

:SELECT_ACTION
cls
echo ==========================================
echo           AXIS ENGINE TOOLS
echo ==========================================
echo  1. Full Rebuild (Clean + Build)
echo  2. Quick Build (Build - FAST)
echo  3. Build Tests (Clean + Build Tests)
echo  4. Compile Scenes (.axs -^> .axsb)
echo ==========================================
set "action_choice="
set /p action_choice="Enter number (Default: 1): "

if "%action_choice%"=="" set action_choice=1

echo %action_choice%| findstr /r "^[1-4]$" >nul
if errorlevel 1 goto RETRY_ACTION

if "%action_choice%"=="1" (
    set "QUICK_BUILD="
    set "ENABLE_TESTS=OFF"
    goto SELECT_COMPILER
)
if "%action_choice%"=="2" (
    set "QUICK_BUILD=1"
    set "ENABLE_TESTS=OFF"
    goto SELECT_BUILD_TYPE
)
if "%action_choice%"=="3" (
    set "QUICK_BUILD=1"
    set "ENABLE_TESTS=ON"
    set "BUILD_TYPE=Debug"
    set "CLEAN_MODE=Soft"
    set "ENABLE_EDITOR=OFF"
    set "BUILD_SAMPLES=OFF"
    goto SELECT_COMPILER
)
if "%action_choice%"=="4" (
    goto COMPILE_SCENES
)

:RETRY_ACTION
echo [ERROR] Invalid selection!
pause
goto SELECT_ACTION

:COMPILE_SCENES
cls
echo ==========================================
echo        COMPILE SCENES (.axs -^> .axsb)
echo ==========================================
echo  1. Compile Single File
echo  2. Compile Directory (Recursive)
echo ------------------------------------------
echo  B. Back to Main Menu
echo ==========================================
set "compile_choice="
set /p compile_choice="Enter choice (Default: 1): "
if "%compile_choice%"=="" set compile_choice=1
if /i "%compile_choice%"=="b" goto SELECT_ACTION

echo.
echo ==========================================
echo           SELECT BUILD TYPE
echo ==========================================
echo  1. Release (Default)
echo  2. Debug
echo ==========================================
set "compile_type_choice="
set /p compile_type_choice="Enter number (Default: 1): "
if "%compile_type_choice%"=="" set compile_type_choice=1
set COMPILE_BUILD_TYPE=Release
if "%compile_type_choice%"=="2" set COMPILE_BUILD_TYPE=Debug

if "%compile_choice%"=="1" (
    cls
    echo ==========================================
    echo         COMPILE SINGLE FILE
    echo ==========================================
    set /p input_file="Enter path to .axs file: "
    set "input_file=!input_file:"=!"
    if not exist "!input_file!" (
        echo [ERROR] Input file does not exist: !input_file!
        pause
        goto COMPILE_SCENES
    )
    set "output_file=!input_file:.axs=.axsb!"
    goto DO_BUILD_AND_COMPILE_FILE
)

if "%compile_choice%"=="2" (
    cls
    echo ==========================================
    echo         COMPILE DIRECTORY
    echo ==========================================
    set /p input_dir="Enter path to folder: "
    set "input_dir=!input_dir:"=!"
    if not exist "!input_dir!" (
        echo [ERROR] Folder does not exist: !input_dir!
        pause
        goto COMPILE_SCENES
    )
    goto DO_BUILD_AND_COMPILE_DIR
)
goto COMPILE_SCENES

:DO_BUILD_AND_COMPILE_FILE
echo.
echo [INFO] Configuring cmake (ENABLE_EDITOR=OFF, BUILD_SAMPLES=OFF, ENABLE_TESTS=OFF)...
cmake -B build -DENABLE_EDITOR=OFF -DBUILD_SAMPLES=OFF -DENABLE_TESTS=OFF
if errorlevel 1 ( echo [ERROR] CMake configuration failed! & pause & goto SELECT_ACTION )
echo [INFO] Building axis_compile in %COMPILE_BUILD_TYPE%...
cmake --build build --config %COMPILE_BUILD_TYPE% --target axis_compile --parallel 4
if errorlevel 1 ( echo [ERROR] Build failed! & pause & goto SELECT_ACTION )
set "axis_compile_exe=build\bin\%COMPILE_BUILD_TYPE%\axis_compile.exe"
if not exist "!axis_compile_exe!" set "axis_compile_exe=build\bin\axis_compile.exe"
if not exist "!axis_compile_exe!" ( echo [ERROR] axis_compile executable not found. & pause & goto SELECT_ACTION )
echo.
echo Compiling !input_file! to !output_file!...
"!axis_compile_exe!" "!input_file!" "!output_file!"
if errorlevel 1 ( echo [ERROR] Compilation failed! ) else ( echo [SUCCESS] Done! )
pause
goto COMPILE_SCENES

:DO_BUILD_AND_COMPILE_DIR
echo.
echo [INFO] Configuring cmake (ENABLE_EDITOR=OFF, BUILD_SAMPLES=OFF, ENABLE_TESTS=OFF)...
cmake -B build -DENABLE_EDITOR=OFF -DBUILD_SAMPLES=OFF -DENABLE_TESTS=OFF
if errorlevel 1 ( echo [ERROR] CMake configuration failed! & pause & goto SELECT_ACTION )
echo [INFO] Building axis_compile in %COMPILE_BUILD_TYPE%...
cmake --build build --config %COMPILE_BUILD_TYPE% --target axis_compile --parallel 4
if errorlevel 1 ( echo [ERROR] Build failed! & pause & goto SELECT_ACTION )
set "axis_compile_exe=build\bin\%COMPILE_BUILD_TYPE%\axis_compile.exe"
if not exist "!axis_compile_exe!" set "axis_compile_exe=build\bin\axis_compile.exe"
if not exist "!axis_compile_exe!" ( echo [ERROR] axis_compile executable not found. & pause & goto SELECT_ACTION )
echo.
echo Compiling all .axs files in !input_dir!...
for /r "!input_dir!" %%f in (*.axs) do (
    set "infile=%%f"
    set "outfile=%%~dpnf.axsb"
    echo Compiling %%~nxf...
    "!axis_compile_exe!" "!infile!" "!outfile!"
)
echo [INFO] Done compilation.
pause
goto COMPILE_SCENES

:SELECT_COMPILER
cls
echo ==========================================
echo       SELECT COMPILER / GENERATOR
echo ==========================================
echo  1. Auto-Detect (Default - CMake will pick latest VS)
echo  2. Visual Studio 2022
echo  3. Visual Studio 2026
echo  4. Visual Studio 2019
echo  5. Visual Studio 2017
echo  6. Visual Studio 2015
echo  7. Visual Studio 2013
echo  8. Visual Studio 11 2012
echo  9. Visual Studio 10 2010
echo 10. Visual Studio 9 2008
echo 11. Visual Studio 8 2005
echo 12. Visual Studio 6 (2000-ish) [Legacy]
echo 13. MinGW (MinGW Makefiles)
echo 14. Clang (Ninja / NMake)
echo ------------------------------------------
echo  B. Back to Main Menu
echo ==========================================
set "comp_choice="
set /p comp_choice="Enter number (Default: 1): "

if "%comp_choice%"=="" set comp_choice=1
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

if "%comp_choice%"=="1" set "GENERATOR="
if "%comp_choice%"=="2" set GENERATOR="Visual Studio 17 2022"
if "%comp_choice%"=="3" set GENERATOR="Visual Studio 18 2026"
if "%comp_choice%"=="4" set GENERATOR="Visual Studio 16 2019"
if "%comp_choice%"=="5" set GENERATOR="Visual Studio 15 2017"
if "%comp_choice%"=="6" set GENERATOR="Visual Studio 14 2015"
if "%comp_choice%"=="7" set GENERATOR="Visual Studio 12 2013"
if "%comp_choice%"=="8" set GENERATOR="Visual Studio 11 2012"
if "%comp_choice%"=="9" set GENERATOR="Visual Studio 10 2010"
if "%comp_choice%"=="10" set GENERATOR="Visual Studio 9 2008"
if "%comp_choice%"=="11" set GENERATOR="Visual Studio 8 2005"
if "%comp_choice%"=="12" set GENERATOR="Visual Studio 6"
if "%comp_choice%"=="13" set GENERATOR="MinGW Makefiles"
if "%comp_choice%"=="14" set GENERATOR="Ninja"

echo Selected Compiler Option: %comp_choice%
if "%ENABLE_TESTS%"=="ON" goto SELECT_BACKENDS

:SELECT_BUILD_TYPE
cls
echo.
echo ==========================================
echo           SELECT BUILD TYPE
echo ==========================================
echo  1. Release (Default)
echo  2. Debug
echo  3. RelWithDebInfo
echo  4. MinSizeRel
echo ------------------------------------------
echo  B. Back
echo ==========================================
set "type_choice="
set /p type_choice="Enter number (Default: 1): "

if "%type_choice%"=="" set type_choice=1
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
if "%type_choice%"=="1" set BUILD_TYPE=Release
if "%type_choice%"=="2" set BUILD_TYPE=Debug
if "%type_choice%"=="3" set BUILD_TYPE=RelWithDebInfo
if "%type_choice%"=="4" set BUILD_TYPE=MinSizeRel

echo Selected Build Type: %BUILD_TYPE%

if defined QUICK_BUILD (
    set "GRAPHICS_BACKEND=OpenGL"
    set "PHYSICS_BACKEND=Bullet"
    set "AUDIO_BACKEND=Null"
    set "CLEAN_MODE=N/A (Quick Build)"
    set "ENABLE_EDITOR=OFF"
    set "BUILD_SAMPLES=OFF"
    set "ENABLE_TESTS=OFF"
    if exist "build\CMakeCache.txt" (
        for /f "tokens=2 delims==" %%i in ('findstr /C:"AXIS_GRAPHICS_BACKEND:STRING=" build\CMakeCache.txt') do set "GRAPHICS_BACKEND=%%i"
        for /f "tokens=2 delims==" %%i in ('findstr /C:"AXIS_PHYSICS_BACKEND:STRING=" build\CMakeCache.txt') do set "PHYSICS_BACKEND=%%i"
        for /f "tokens=2 delims==" %%i in ('findstr /C:"AXIS_AUDIO_BACKEND:STRING=" build\CMakeCache.txt') do set "AUDIO_BACKEND=%%i"
        for /f "tokens=2 delims==" %%i in ('findstr /C:"ENABLE_EDITOR:BOOL=" build\CMakeCache.txt') do set "ENABLE_EDITOR=%%i"
        for /f "tokens=2 delims==" %%i in ('findstr /C:"BUILD_SAMPLES:BOOL=" build\CMakeCache.txt') do set "BUILD_SAMPLES=%%i"
        for /f "tokens=2 delims==" %%i in ('findstr /C:"ENABLE_TESTS:BOOL=" build\CMakeCache.txt') do set "ENABLE_TESTS=%%i"
    )
    goto CONFIRM_CONFIG
)
goto SELECT_CLEAN_MODE

:SELECT_CLEAN_MODE
cls
echo.
echo ==========================================
echo           SELECT CLEAN MODE
echo ==========================================
echo  1. Soft   (Warning only if cannot clean) [Default]
echo  2. Strict (Fail if cannot clean build/bin)
echo ------------------------------------------
echo  B. Back
echo ==========================================
set "clean_mode_choice="
set /p clean_mode_choice="Enter number (Default: 1): "

if "%clean_mode_choice%"=="" set clean_mode_choice=1
if /i "%clean_mode_choice%"=="b" goto SELECT_BUILD_TYPE

echo %clean_mode_choice%| findstr /r "^[1-2]$" >nul
if errorlevel 1 goto RETRY_CLEAN_MODE

set CLEAN_MODE=Soft
if "%clean_mode_choice%"=="1" set CLEAN_MODE=Soft
if "%clean_mode_choice%"=="2" set CLEAN_MODE=Strict

echo Selected Clean Mode: %CLEAN_MODE%
goto SELECT_EDITOR

:RETRY_CLEAN_MODE
echo [ERROR] Invalid selection!
pause
goto SELECT_CLEAN_MODE

:SELECT_EDITOR
cls
echo.
echo ==========================================
echo           ENABLE EDITOR?
echo ==========================================
echo  1. No (Default)
echo  2. Yes (Enables Editor + ImGui)
echo ------------------------------------------
echo  B. Back
echo ==========================================
set "editor_choice="
set /p editor_choice="Enter number (Default: 1): "

if "%editor_choice%"=="" set editor_choice=1
if /i "%editor_choice%"=="b" goto SELECT_CLEAN_MODE

echo %editor_choice%| findstr /r "^[1-2]$" >nul
if errorlevel 1 goto RETRY_EDITOR

set ENABLE_EDITOR=OFF
if "%editor_choice%"=="2" set ENABLE_EDITOR=ON

if "%ENABLE_EDITOR%"=="ON" goto SELECT_BUILD_SAMPLES
set BUILD_SAMPLES=OFF
goto SELECT_BACKENDS

:RETRY_EDITOR
echo [ERROR] Invalid selection!
pause
goto SELECT_EDITOR

:SELECT_BUILD_SAMPLES
cls
echo.
echo ==========================================
echo           BUILD SAMPLES?
echo ==========================================
echo  1. Yes (Build axis_samples) [Default]
echo  2. No  (Build engine + editor libs only)
echo ------------------------------------------
echo  B. Back
echo ==========================================
set "sample_choice="
set /p sample_choice="Enter number (Default: 1): "

if "%sample_choice%"=="" set sample_choice=1
if /i "%sample_choice%"=="b" goto SELECT_EDITOR

echo %sample_choice%| findstr /r "^[1-2]$" >nul
if errorlevel 1 goto RETRY_BUILD_SAMPLES

set BUILD_SAMPLES=ON
if "%sample_choice%"=="2" set BUILD_SAMPLES=OFF

goto SELECT_BACKENDS

:RETRY_BUILD_SAMPLES
echo [ERROR] Invalid selection!
pause
goto SELECT_BUILD_SAMPLES

:SELECT_BACKENDS
cls
echo.
echo ==========================================
echo           SELECT ENGINE BACKENDS
echo ==========================================
echo Graphics Backend:
echo  1. OpenGL (Default)
echo  2. Vulkan [Not implemented]
echo  3. DirectX [Not implemented]
echo ------------------------------------------
echo  B. Back
echo ==========================================
set "graphics_choice="
set /p graphics_choice="Enter number (Default: 1): "

if "%graphics_choice%"=="" set graphics_choice=1
if /i "%graphics_choice%"=="b" (
    if "%ENABLE_TESTS%"=="ON" goto SELECT_COMPILER
    if defined QUICK_BUILD goto SELECT_BUILD_TYPE
    if "%ENABLE_EDITOR%"=="ON" goto SELECT_BUILD_SAMPLES
    goto SELECT_EDITOR
)

echo %graphics_choice%| findstr /r "^[1-3]$" >nul
if errorlevel 1 goto RETRY_BACKENDS

set GRAPHICS_BACKEND=OpenGL
if "%graphics_choice%"=="2" set GRAPHICS_BACKEND=Vulkan
if "%graphics_choice%"=="3" set GRAPHICS_BACKEND=DirectX

echo.
echo Physics Backend:
echo  1. Bullet (Default)
echo  2. PhysX [Not implemented]
set "physics_choice="
set /p physics_choice="Enter number (Default: 1): "

if "%physics_choice%"=="" set physics_choice=1
echo %physics_choice%| findstr /r "^[1-2]$" >nul
if errorlevel 1 goto RETRY_BACKENDS

set PHYSICS_BACKEND=Bullet
if "%physics_choice%"=="2" set PHYSICS_BACKEND=PhysX

echo.
echo Audio Backend:
echo  1. Null (Default, no audio output)
echo  2. FMOD (requires FMOD SDK / FMOD_ROOT_DIR)
echo  3. IrrKlang (requires irrKlang SDK / IRRKLANG_ROOT_DIR)
set "audio_choice="
set /p audio_choice="Enter number (Default: 1): "

if "%audio_choice%"=="" set audio_choice=1
echo %audio_choice%| findstr /r "^[1-3]$" >nul
if errorlevel 1 goto RETRY_BACKENDS

set AUDIO_BACKEND=Null
if "%audio_choice%"=="2" set AUDIO_BACKEND=FMOD
if "%audio_choice%"=="3" set AUDIO_BACKEND=IrrKlang

call :VALIDATE_AUDIO_BACKEND
if errorlevel 1 goto SELECT_BACKENDS

goto CONFIRM_CONFIG

:VALIDATE_AUDIO_BACKEND
if /i "%AUDIO_BACKEND%"=="IrrKlang" (
    set "IRRKLANG_SDK_FOUND="
    if defined IRRKLANG_ROOT_DIR (
        if exist "!IRRKLANG_ROOT_DIR!\include\irrKlang.h" set "IRRKLANG_SDK_FOUND=1"
        if exist "!IRRKLANG_ROOT_DIR!\irrKlang.h" set "IRRKLANG_SDK_FOUND=1"
        if exist "!IRRKLANG_ROOT_DIR!\irrKlang\irrKlang.h" set "IRRKLANG_SDK_FOUND=1"
    )
    if exist "C:\Program Files\irrKlang\include\irrKlang.h" set "IRRKLANG_SDK_FOUND=1"
    if exist "C:\Program Files (x86)\irrKlang\include\irrKlang.h" set "IRRKLANG_SDK_FOUND=1"
    if not defined IRRKLANG_SDK_FOUND (
        echo.
        echo [WARN] IrrKlang SDK not found. Set IRRKLANG_ROOT_DIR or choose Null audio.
        set "audio_fallback="
        set /p audio_fallback="Fallback to Null audio? (Y/N) [Default: Y]: "
        if "!audio_fallback!"=="" set "audio_fallback=Y"
        if /i "!audio_fallback!"=="Y" (
            set "AUDIO_BACKEND=Null"
            exit /b 0
        )
        exit /b 1
    )
)
if /i "%AUDIO_BACKEND%"=="FMOD" (
    set "FMOD_SDK_FOUND="
    if defined FMOD_ROOT_DIR (
        if exist "!FMOD_ROOT_DIR!\api\core\inc\fmod.hpp" set "FMOD_SDK_FOUND=1"
        if exist "!FMOD_ROOT_DIR!\inc\fmod.hpp" set "FMOD_SDK_FOUND=1"
        if exist "!FMOD_ROOT_DIR!\include\fmod.hpp" set "FMOD_SDK_FOUND=1"
    )
    if exist "C:\Program Files\FMOD SoundSystem\FMOD Studio API Windows\api\core\inc\fmod.hpp" set "FMOD_SDK_FOUND=1"
    if exist "C:\Program Files (x86)\FMOD SoundSystem\FMOD Studio API Windows\api\core\inc\fmod.hpp" set "FMOD_SDK_FOUND=1"
    if not defined FMOD_SDK_FOUND (
        echo.
        echo [WARN] FMOD SDK not found. Set FMOD_ROOT_DIR or choose Null audio.
        set "audio_fallback="
        set /p audio_fallback="Fallback to Null audio? (Y/N) [Default: Y]: "
        if "!audio_fallback!"=="" set "audio_fallback=Y"
        if /i "!audio_fallback!"=="Y" (
            set "AUDIO_BACKEND=Null"
            exit /b 0
        )
        exit /b 1
    )
)
exit /b 0

:RETRY_BACKENDS
echo [ERROR] Invalid backend selection!
pause
goto SELECT_BACKENDS

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
echo  Graphics:   %GRAPHICS_BACKEND%
echo  Physics:    %PHYSICS_BACKEND%
echo  Audio:      %AUDIO_BACKEND%
echo  Clean Mode: %CLEAN_MODE%
echo  Editor:     %ENABLE_EDITOR%
if "%ENABLE_EDITOR%"=="ON" echo  Samples:    %BUILD_SAMPLES%
echo  Tests:      %ENABLE_TESTS%
echo ==========================================
set "confirm="
set /p confirm="Do you want to proceed? (Y/N, B=Back) [Default: Y]: "
if "%confirm%"=="" set confirm=y

if /i "%confirm%"=="b" (
    if defined QUICK_BUILD (
        goto SELECT_BUILD_TYPE
    ) else (
        goto SELECT_BACKENDS
    )
)
if /i "%confirm%"=="n" goto SELECT_ACTION
if /i "%confirm%"=="y" (
    cls
    goto CLEAN_FOLDERS
)
goto CONFIRM_CONFIG

:CLEAN_FOLDERS
echo.
echo ==========================================
echo        CLEANING BIN AND BUILD...
echo ==========================================

taskkill /F /IM MyGame.exe >nul 2>&1
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
    if exist "bin\MyGame.exe" (
        del /f /q "bin\MyGame.exe"
    )
)

:SKIP_CLEAN

title Axis Engine Tools - Building...
echo.
echo ==========================================
echo      CONFIGURING AND BUILDING...
echo ==========================================

where cmake >nul 2>&1
if not errorlevel 1 (
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

set "DEPENDENCY_CMAKE_FLAGS="
if defined VCPKG_ROOT (
    set "VCPKG_TOOLCHAIN=!VCPKG_ROOT!\scripts\buildsystems\vcpkg.cmake"
    if exist "!VCPKG_TOOLCHAIN!" (
        set "DEPENDENCY_CMAKE_FLAGS=-DCMAKE_TOOLCHAIN_FILE=!VCPKG_TOOLCHAIN!"
        if exist "cmake\vcpkg-overlay-ports" (
            set "DEPENDENCY_CMAKE_FLAGS=!DEPENDENCY_CMAKE_FLAGS! -DVCPKG_OVERLAY_PORTS=%CD%\cmake\vcpkg-overlay-ports"
        )
        echo [INFO] Using vcpkg toolchain: !VCPKG_TOOLCHAIN!
    )
)

if not defined QUICK_BUILD (
    set "BACKEND_CMAKE_FLAGS=-DAXIS_GRAPHICS_BACKEND=!GRAPHICS_BACKEND! -DAXIS_PHYSICS_BACKEND=!PHYSICS_BACKEND! -DAXIS_AUDIO_BACKEND=!AUDIO_BACKEND!"
    if not defined GENERATOR (
        "!CMAKE_CMD!" -B build -DENABLE_EDITOR=!ENABLE_EDITOR! -DBUILD_SAMPLES=!BUILD_SAMPLES! -DENABLE_TESTS=!ENABLE_TESTS! !BACKEND_CMAKE_FLAGS! !DEPENDENCY_CMAKE_FLAGS!
    ) else (
        "!CMAKE_CMD!" -G %GENERATOR% -B build -DENABLE_EDITOR=!ENABLE_EDITOR! -DBUILD_SAMPLES=!BUILD_SAMPLES! -DENABLE_TESTS=!ENABLE_TESTS! !BACKEND_CMAKE_FLAGS! !DEPENDENCY_CMAKE_FLAGS!
    )
) else (
    set "BACKEND_CMAKE_FLAGS="
    if "%ENABLE_TESTS%"=="ON" (
        if not defined GENERATOR (
            "!CMAKE_CMD!" -B build -DENABLE_EDITOR=!ENABLE_EDITOR! -DBUILD_SAMPLES=!BUILD_SAMPLES! -DENABLE_TESTS=!ENABLE_TESTS! !BACKEND_CMAKE_FLAGS! !DEPENDENCY_CMAKE_FLAGS!
        ) else (
            "!CMAKE_CMD!" -G %GENERATOR% -B build -DENABLE_EDITOR=!ENABLE_EDITOR! -DBUILD_SAMPLES=!BUILD_SAMPLES! -DENABLE_TESTS=!ENABLE_TESTS! !BACKEND_CMAKE_FLAGS! !DEPENDENCY_CMAKE_FLAGS!
        )
    ) else (
        if not defined GENERATOR (
            "!CMAKE_CMD!" -B build -DENABLE_TESTS=!ENABLE_TESTS! !BACKEND_CMAKE_FLAGS! !DEPENDENCY_CMAKE_FLAGS!
        ) else (
            "!CMAKE_CMD!" -G %GENERATOR% -B build -DENABLE_TESTS=!ENABLE_TESTS! !BACKEND_CMAKE_FLAGS! !DEPENDENCY_CMAKE_FLAGS!
        )
    )
)

if errorlevel 1 (
    echo.
    echo [ERROR] CMake Configuration failed!
    pause
    goto EXIT_SCRIPT
)

"!CMAKE_CMD!" --build build --config %BUILD_TYPE%

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    pause
    goto EXIT_SCRIPT
)

:RUN_GAME
echo.
echo ==========================================
echo        BUILD SUCCESS: LIBS GENERATED
echo ==========================================
echo Copying engine and dependency libraries to '%CD%\build\lib' and '%CD%\build\bin'...

powershell -NoProfile -Command ^
    "$BuildType='%BUILD_TYPE%';" ^
    "$TargetLib='%CD%\build\lib';" ^
    "$TargetBin='%CD%\build\bin';" ^
    "$VcpkgSub = if ($BuildType -eq 'Debug') { 'x64-windows\debug' } else { 'x64-windows' };" ^
    "New-Item -ItemType Directory -Force -Path $TargetLib, $TargetBin | Out-Null;" ^
    "if (Test-Path \"vcpkg_installed\$VcpkgSub\lib\*.lib\") { Copy-Item \"vcpkg_installed\$VcpkgSub\lib\*.lib\" $TargetLib -Force };" ^
    "if (Test-Path \"vcpkg_installed\$VcpkgSub\bin\*.dll\") { Copy-Item \"vcpkg_installed\$VcpkgSub\bin\*.dll\" $TargetBin -Force };" ^
    "Write-Host 'Libraries and DLLs copied successfully to AxisEngine/build/lib/ and AxisEngine/build/bin/!';"

:EXIT_SCRIPT
exit /b %ERRORLEVEL%
