#!/bin/bash
# ==========================================
#          AXIS ENGINE BUILDER (UNIX)
# ==========================================

set -e

# ANSI Color Codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Detect OS
OS_NAME=$(uname -s)
IS_MAC=false
if [ "$OS_NAME" = "Darwin" ]; then
    IS_MAC=true
fi

# Trap Ctrl+C to exit cleanly
trap 'echo -e "\n${RED}[ABORTED]${NC} Build process interrupted by user."; exit 1' INT

# Help function
show_help() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  -a, --action <1-3>      1: Full Rebuild, 2: Quick Build, 3: Build Tests"
    echo "  -c, --compiler <1-4>    1: Auto-Detect, 2: Unix Makefiles, 3: Ninja, 4: Xcode (macOS)"
    echo "  -t, --type <1-4>        1: Release, 2: Debug, 3: RelWithDebInfo, 4: MinSizeRel"
    echo "  -e, --editor <yes/no>   Enable or disable Editor (ImGui)"
    echo "  -s, --samples <yes/no>  Build samples (only if Editor is enabled)"
    echo "  --graphics <backend>    Graphics backend: OpenGL, Vulkan, DirectX, or Null"
    echo "  --physics <backend>     Physics backend: Bullet or PhysX"
    echo "  --audio <backend>       Audio backend: Null, IrrKlang, or FMOD"
    echo "  -y, --yes               Skip confirmation prompts"
    echo "  -h, --help              Show this help message"
}

# Parse command line args for non-interactive automation
SKIP_PROMPTS=false
while [[ "$#" -gt 0 ]]; do
    case $1 in
        -a|--action) ACTION_CHOICE="$2"; shift ;;
        -c|--compiler) COMPILER_CHOICE="$2"; shift ;;
        -t|--type) TYPE_CHOICE="$2"; shift ;;
        -e|--editor) 
            if [ "$2" = "yes" ]; then ENABLE_EDITOR="ON"; else ENABLE_EDITOR="OFF"; fi
            shift ;;
        -s|--samples) 
            if [ "$2" = "yes" ]; then BUILD_SAMPLES="ON"; else BUILD_SAMPLES="OFF"; fi
            shift ;;
        --graphics) AXIS_GRAPHICS_BACKEND="$2"; shift ;;
        --physics) AXIS_PHYSICS_BACKEND="$2"; shift ;;
        --audio) AXIS_AUDIO_BACKEND="$2"; shift ;;
        -y|--yes) SKIP_PROMPTS=true ;;
        -h|--help) show_help; exit 0 ;;
        *) echo "Unknown parameter passed: $1"; show_help; exit 1 ;;
    esac
    shift
done

# Check CMake availability
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}[ERROR] CMake is not installed or not in PATH!${NC}"
    exit 1
fi

# 1. SELECT ACTION
if [ -z "$ACTION_CHOICE" ]; then
    clear
    echo "=========================================="
    echo "           GAME ENGINE LAUNCHER"
    echo "=========================================="
    echo " 1. Full Rebuild (Clean + Build)"
    echo " 2. Quick Build (Build - FAST)"
    echo " 3. Build Tests (Clean + Build Tests)"
    echo "=========================================="
    read -p "Enter number (Default: 1): " ACTION_CHOICE
    [ -z "$ACTION_CHOICE" ] && ACTION_CHOICE=1
fi

case "$ACTION_CHOICE" in
    1)
        QUICK_BUILD=false
        ENABLE_TESTS="OFF"
        ;;
    2)
        QUICK_BUILD=true
        ENABLE_TESTS="OFF"
        ;;
    3)
        QUICK_BUILD=true
        ENABLE_TESTS="ON"
        BUILD_TYPE="Debug"
        CLEAN_MODE="Soft"
        ENABLE_EDITOR="OFF"
        BUILD_SAMPLES="OFF"
        ;;
    *)
        echo -e "${RED}[ERROR] Invalid action selection!${NC}"
        exit 1
        ;;
esac

# 2. SELECT COMPILER / GENERATOR (Skip if Quick Build + Action 2 and variables are set)
if [ -z "$COMPILER_CHOICE" ] && [ "$ACTION_CHOICE" != "2" ]; then
    clear
    echo "=========================================="
    echo "       SELECT COMPILER / GENERATOR"
    echo "=========================================="
    echo " 1. Auto-Detect (Default)"
    echo " 2. Unix Makefiles"
    echo " 3. Ninja"
    if $IS_MAC; then
        echo " 4. Xcode"
    fi
    echo "=========================================="
    read -p "Enter number (Default: 1): " COMPILER_CHOICE
    [ -z "$COMPILER_CHOICE" ] && COMPILER_CHOICE=1
fi

GENERATOR=""
case "$COMPILER_CHOICE" in
    1) GENERATOR="" ;;
    2) GENERATOR="Unix Makefiles" ;;
    3)
        if ! command -v ninja &> /dev/null; then
            echo -e "${YELLOW}[WARNING] Ninja not found! Falling back to Unix Makefiles.${NC}"
            GENERATOR="Unix Makefiles"
        else
            GENERATOR="Ninja"
        fi
        ;;
    4)
        if $IS_MAC; then
            GENERATOR="Xcode"
        else
            echo -e "${RED}[ERROR] Xcode generator is only available on macOS!${NC}"
            exit 1
        fi
        ;;
    "") GENERATOR="" ;;
    *)
        echo -e "${RED}[ERROR] Invalid compiler selection!${NC}"
        exit 1
        ;;
esac

# 3. SELECT BUILD TYPE
if [ -z "$TYPE_CHOICE" ] && [ "$ENABLE_TESTS" != "ON" ]; then
    clear
    echo "=========================================="
    echo "           SELECT BUILD TYPE"
    echo "=========================================="
    echo " 1. Release (Default)"
    echo " 2. Debug"
    echo " 3. RelWithDebInfo"
    echo " 4. MinSizeRel"
    echo "=========================================="
    read -p "Enter number (Default: 1): " TYPE_CHOICE
    [ -z "$TYPE_CHOICE" ] && TYPE_CHOICE=1
fi

if [ "$ENABLE_TESTS" != "ON" ]; then
    case "$TYPE_CHOICE" in
        1) BUILD_TYPE="Release" ;;
        2) BUILD_TYPE="Debug" ;;
        3) BUILD_TYPE="RelWithDebInfo" ;;
        4) BUILD_TYPE="MinSizeRel" ;;
        *) BUILD_TYPE="Release" ;;
    esac
fi

# 4. SELECT CLEAN MODE
if [ "$QUICK_BUILD" = "false" ] && [ -z "$CLEAN_MODE" ]; then
    clear
    echo "=========================================="
    echo "           SELECT CLEAN MODE"
    echo "=========================================="
    echo " 1. Soft   (Warning only if cannot clean) [Default]"
    echo " 2. Strict (Fail if cannot clean build)"
    echo "=========================================="
    read -p "Enter number (Default: 1): " CLEAN_CHOICE
    [ -z "$CLEAN_CHOICE" ] && CLEAN_CHOICE=1
    if [ "$CLEAN_CHOICE" = "1" ]; then
        CLEAN_MODE="Soft"
    else
        CLEAN_MODE="Strict"
    fi
fi

# 5. SELECT EDITOR
if [ "$ENABLE_TESTS" != "ON" ] && [ -z "$ENABLE_EDITOR" ]; then
    clear
    echo "=========================================="
    echo "           ENABLE EDITOR?"
    echo "=========================================="
    echo " 1. No (Default)"
    echo " 2. Yes (Enables Editor + ImGui)"
    echo "=========================================="
    read -p "Enter number (Default: 1): " EDITOR_CHOICE
    [ -z "$EDITOR_CHOICE" ] && EDITOR_CHOICE=1
    if [ "$EDITOR_CHOICE" = "2" ]; then
        ENABLE_EDITOR="ON"
    else
        ENABLE_EDITOR="OFF"
    fi
fi

# 6. SELECT BUILD SAMPLES
if [ "$ENABLE_EDITOR" = "ON" ] && [ "$ENABLE_TESTS" != "ON" ] && [ -z "$BUILD_SAMPLES" ]; then
    clear
    echo "=========================================="
    echo "           BUILD SAMPLES?"
    echo "=========================================="
    echo " 1. Yes (Build axis_samples) [Default]"
    echo " 2. No  (Build engine + editor libs only)"
    echo "=========================================="
    read -p "Enter number (Default: 1): " SAMPLES_CHOICE
    [ -z "$SAMPLES_CHOICE" ] && SAMPLES_CHOICE=1
    if [ "$SAMPLES_CHOICE" = "2" ]; then
        BUILD_SAMPLES="OFF"
    else
        BUILD_SAMPLES="ON"
    fi
elif [ -z "$BUILD_SAMPLES" ]; then
    BUILD_SAMPLES="OFF"
fi

# 7. SELECT BACKENDS
canonical_backend() {
    local value
    value=$(echo "$1" | tr '[:upper:]' '[:lower:]')
    case "$value" in
        opengl) echo "OpenGL" ;;
        vulkan) echo "Vulkan" ;;
        directx|dx|d3d) echo "DirectX" ;;
        bullet) echo "Bullet" ;;
        physx) echo "PhysX" ;;
        null|none) echo "Null" ;;
        irrklang) echo "IrrKlang" ;;
        fmod) echo "FMOD" ;;
        *) echo "" ;;
    esac
}

if [ -z "$AXIS_GRAPHICS_BACKEND" ]; then
    clear
    echo "=========================================="
    echo "          SELECT GRAPHICS BACKEND"
    echo "=========================================="
    echo " 1. OpenGL (Default)"
    echo " 2. Vulkan"
    echo " 3. DirectX"
    echo " 4. Null"
    echo "=========================================="
    read -p "Enter number (Default: 1): " GRAPHICS_CHOICE
    [ -z "$GRAPHICS_CHOICE" ] && GRAPHICS_CHOICE=1
    case "$GRAPHICS_CHOICE" in
        1) AXIS_GRAPHICS_BACKEND="OpenGL" ;;
        2) AXIS_GRAPHICS_BACKEND="Vulkan" ;;
        3) AXIS_GRAPHICS_BACKEND="DirectX" ;;
        4) AXIS_GRAPHICS_BACKEND="Null" ;;
        *) echo -e "${RED}[ERROR] Invalid graphics backend selection!${NC}"; exit 1 ;;
    esac
else
    AXIS_GRAPHICS_BACKEND=$(canonical_backend "$AXIS_GRAPHICS_BACKEND")
fi

if [ -z "$AXIS_PHYSICS_BACKEND" ]; then
    clear
    echo "=========================================="
    echo "          SELECT PHYSICS BACKEND"
    echo "=========================================="
    echo " 1. Bullet (Default)"
    echo " 2. PhysX"
    echo "=========================================="
    read -p "Enter number (Default: 1): " PHYSICS_CHOICE
    [ -z "$PHYSICS_CHOICE" ] && PHYSICS_CHOICE=1
    case "$PHYSICS_CHOICE" in
        1) AXIS_PHYSICS_BACKEND="Bullet" ;;
        2) AXIS_PHYSICS_BACKEND="PhysX" ;;
        *) echo -e "${RED}[ERROR] Invalid physics backend selection!${NC}"; exit 1 ;;
    esac
else
    AXIS_PHYSICS_BACKEND=$(canonical_backend "$AXIS_PHYSICS_BACKEND")
fi

if [ -z "$AXIS_AUDIO_BACKEND" ]; then
    clear
    echo "=========================================="
    echo "           SELECT AUDIO BACKEND"
    echo "=========================================="
    echo " 1. Null (Default, no audio output)"
    echo " 2. FMOD (requires FMOD SDK / FMOD_ROOT_DIR)"
    echo " 3. IrrKlang (requires irrKlang SDK)"
    echo "=========================================="
    read -p "Enter number (Default: 1): " AUDIO_CHOICE
    [ -z "$AUDIO_CHOICE" ] && AUDIO_CHOICE=1
    case "$AUDIO_CHOICE" in
        1) AXIS_AUDIO_BACKEND="Null" ;;
        2) AXIS_AUDIO_BACKEND="FMOD" ;;
        3) AXIS_AUDIO_BACKEND="IrrKlang" ;;
        *) echo -e "${RED}[ERROR] Invalid audio backend selection!${NC}"; exit 1 ;;
    esac
else
    AXIS_AUDIO_BACKEND=$(canonical_backend "$AXIS_AUDIO_BACKEND")
fi

if [ -z "$AXIS_GRAPHICS_BACKEND" ] || [ -z "$AXIS_PHYSICS_BACKEND" ] || [ -z "$AXIS_AUDIO_BACKEND" ]; then
    echo -e "${RED}[ERROR] Invalid backend argument.${NC}"
    exit 1
fi

audio_sdk_available() {
    case "$AXIS_AUDIO_BACKEND" in
        Null) return 0 ;;
        IrrKlang)
            [ -n "$IRRKLANG_ROOT_DIR" ] && {
                [ -f "$IRRKLANG_ROOT_DIR/include/irrKlang.h" ] ||
                [ -f "$IRRKLANG_ROOT_DIR/irrKlang.h" ] ||
                [ -f "$IRRKLANG_ROOT_DIR/irrKlang/irrKlang.h" ]
            } && return 0
            [ -f "/usr/include/irrKlang.h" ] || [ -f "/usr/local/include/irrKlang.h" ] && return 0
            return 1
            ;;
        FMOD)
            [ -n "$FMOD_ROOT_DIR" ] && {
                [ -f "$FMOD_ROOT_DIR/api/core/inc/fmod.hpp" ] ||
                [ -f "$FMOD_ROOT_DIR/inc/fmod.hpp" ] ||
                [ -f "$FMOD_ROOT_DIR/include/fmod.hpp" ]
            } && return 0
            [ -f "/opt/fmod/api/core/inc/fmod.hpp" ] || [ -f "/usr/include/fmod.hpp" ] || [ -f "/usr/local/include/fmod.hpp" ] && return 0
            return 1
            ;;
    esac
}

if ! audio_sdk_available; then
    echo -e "${YELLOW}[WARNING]${NC} $AXIS_AUDIO_BACKEND SDK not found."
    if [ "$SKIP_PROMPTS" = "true" ]; then
        echo -e "${RED}[ERROR]${NC} Set ${AXIS_AUDIO_BACKEND}_ROOT_DIR or use --audio Null."
        exit 1
    fi
    read -p "Fallback to Null audio? (Y/N) [Default: Y]: " AUDIO_FALLBACK
    [ -z "$AUDIO_FALLBACK" ] && AUDIO_FALLBACK="y"
    if [[ "$AUDIO_FALLBACK" =~ ^[Yy]$ ]]; then
        AXIS_AUDIO_BACKEND="Null"
    else
        exit 1
    fi
fi

# 8. CONFIRM CONFIGURATION
if [ "$SKIP_PROMPTS" = "false" ]; then
    clear
    echo "=========================================="
    echo "           CONFIRM CONFIGURATION"
    echo "=========================================="
    if [ -z "$GENERATOR" ]; then
        echo -e "  Generator:  Default (Auto-Detect)"
    else
        echo -e "  Generator:  $GENERATOR"
    fi
    echo -e "  Build Type: $BUILD_TYPE"
    echo -e "  Graphics:   $AXIS_GRAPHICS_BACKEND"
    echo -e "  Physics:    $AXIS_PHYSICS_BACKEND"
    echo -e "  Audio:      $AXIS_AUDIO_BACKEND"
    if [ "$QUICK_BUILD" = "true" ]; then
        echo -e "  Build Mode: QUICK"
    else
        echo -e "  Clean Mode: $CLEAN_MODE"
        echo -e "  Editor:     $ENABLE_EDITOR"
        if [ "$ENABLE_EDITOR" = "ON" ]; then
            echo -e "  Samples:    $BUILD_SAMPLES"
        fi
        echo -e "  Tests:      $ENABLE_TESTS"
    fi
    echo "=========================================="
    read -p "Do you want to proceed? (Y/N) [Default: Y]: " CONFIRM
    [ -z "$CONFIRM" ] && CONFIRM="y"
    if [[ "$CONFIRM" =~ ^[Nn]$ ]]; then
        echo -e "${YELLOW}[ABORTED]${NC} Build cancelled by user."
        exit 0
    fi
fi

# CLEAN FOLDERS
echo -e "\n=========================================="
echo -e "        CLEANING BIN AND BUILD..."
echo -e "=========================================="

# Terminate running instances
pkill -f axis_samples 2>/dev/null || true
pkill -f ninja 2>/dev/null || true

if [ "$QUICK_BUILD" = "true" ]; then
    echo -e "${GREEN}[SKIP]${NC} Quick Build enabled. Skipping folder deletion..."
else
    if [ -d "build" ]; then
        echo "Deleting build folder..."
        rm -rf build
        if [ -d "build" ]; then
            echo -e "${RED}[FAILED]${NC} Could not delete 'build' folder."
            if [ "$CLEAN_MODE" = "Strict" ]; then
                echo -e "${RED}[ERROR] Strict Mode enabled. Aborting.${NC}"
                exit 1
            fi
        else
            echo -e "${GREEN}[DELETED]${NC} 'build' folder."
        fi
    fi

    # Clean legacy directories in root if they exist
    if [ -d "bin/$BUILD_TYPE" ]; then
        echo "Deleting bin/$BUILD_TYPE folder..."
        rm -rf "bin/$BUILD_TYPE"
    fi
fi

# CONFIGURING AND BUILDING
echo -e "\n=========================================="
echo -e "      CONFIGURING AND BUILDING..."
echo -e "=========================================="

CMAKE_FLAGS=(
    "-DENABLE_EDITOR=$ENABLE_EDITOR"
    "-DBUILD_SAMPLES=$BUILD_SAMPLES"
    "-DENABLE_TESTS=$ENABLE_TESTS"
    "-DAXIS_GRAPHICS_BACKEND=$AXIS_GRAPHICS_BACKEND"
    "-DAXIS_PHYSICS_BACKEND=$AXIS_PHYSICS_BACKEND"
    "-DAXIS_AUDIO_BACKEND=$AXIS_AUDIO_BACKEND"
)

if [ -n "$VCPKG_ROOT" ] && [ -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then
    CMAKE_FLAGS+=("-DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake")
    if [ -d "cmake/vcpkg-overlay-ports" ]; then
        CMAKE_FLAGS+=("-DVCPKG_OVERLAY_PORTS=$PWD/cmake/vcpkg-overlay-ports")
    fi
    echo -e "${BLUE}[INFO]${NC} Using vcpkg toolchain: $VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
fi

# For single-config generators, we must supply the build type during configure
if [ -n "$BUILD_TYPE" ]; then
    CMAKE_FLAGS+=("-DCMAKE_BUILD_TYPE=$BUILD_TYPE")
fi

if [ -n "$GENERATOR" ]; then
    cmake -G "$GENERATOR" "${CMAKE_FLAGS[@]}" -B build
else
    cmake "${CMAKE_FLAGS[@]}" -B build
fi

# Build step
cmake --build build --config "$BUILD_TYPE"

echo -e "\n=========================================="
echo -e "        BUILD SUCCESS: LIBS GENERATED"
echo -e "=========================================="
echo -e "Libraries (libaxis_engine.a, etc.) are located in: build/lib/"
