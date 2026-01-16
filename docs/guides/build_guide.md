# Build Guide

This guide details how to build the AXIS Engine from source.

## Prerequisites

*   **Build System**: CMake 3.20+
*   **Compiler**: C++20 compliant compiler.
    *   **Recommended**: Visual Studio 2019/2022 (MSVC)
    *   **Supported**: MinGW-w64 (GCC), Clang (via Ninja/Makefiles)
*   **Note on Libraries**: The project includes pre-compiled DLLs for MSVC (`vc143`). If using MinGW or Clang, you may need to provide compatible binaries for dependencies (Assimp, Bullet, etc.).

## Quick Build (Windows)
The included `build_engine.bat` supports multiple generators.
1. Run `build_engine.bat`.
2. Select **Compiler/Generator** from the menu (e.g., Option 12 for MinGW, 13 for Clang).
3. Follow the prompts.

This script will:
1.  Create a `build` directory.
2.  Run CMake to generate the Visual Studio solution.
3.  Compile the engine in `Release` mode.
4.  Copy necessary DLLs to the `bin/` output folder.

## Manual Build

If you prefer to build manually via command line:

```bash
# 1. Create build directory
mkdir build
cd build

# 2. Configure (Generate VS Solution)
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build . --config Release
```

## Running the Engine

After a successful build, the executable `GameEngine.exe` will be located in:

*   **Automatic Build**: `bin/`
*   **Manual Build**: `build/Release/` or `bin/` depending on CMake config.

Make sure the `assets/` and `resources/` directories are in the same folder as the executable or the working directory is set correctly in Visual Studio.
