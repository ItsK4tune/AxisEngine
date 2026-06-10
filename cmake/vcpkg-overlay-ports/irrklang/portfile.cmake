if(NOT VCPKG_TARGET_IS_WINDOWS)
    message(FATAL_ERROR "The AxisEngine irrKlang overlay port currently supports Windows only.")
endif()

if(NOT DEFINED ENV{IRRKLANG_ROOT_DIR})
    message(FATAL_ERROR "IRRKLANG_ROOT_DIR is not set. Install the irrKlang SDK and set IRRKLANG_ROOT_DIR to its root.")
endif()

set(IRRKLANG_ROOT "$ENV{IRRKLANG_ROOT_DIR}")
set(IRRKLANG_ARCH_DIR "winx64-visualStudio")
if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(IRRKLANG_ARCH_DIR "win32-visualStudio")
endif()

find_path(IRRKLANG_INCLUDE_DIR
    NAMES irrKlang.h
    PATHS
        "${IRRKLANG_ROOT}/include"
        "${IRRKLANG_ROOT}/include/irrKlang"
        "${IRRKLANG_ROOT}/irrKlang"
        "${IRRKLANG_ROOT}"
    NO_DEFAULT_PATH
)

find_file(IRRKLANG_IMPLIB
    NAMES irrKlang.lib
    PATHS
        "${IRRKLANG_ROOT}/lib/${IRRKLANG_ARCH_DIR}"
        "${IRRKLANG_ROOT}/lib/Winx64-visualStudio"
        "${IRRKLANG_ROOT}/lib/Win32-visualStudio"
        "${IRRKLANG_ROOT}/lib"
    NO_DEFAULT_PATH
)

find_file(IRRKLANG_RUNTIME
    NAMES irrKlang.dll
    PATHS
        "${IRRKLANG_ROOT}/bin/${IRRKLANG_ARCH_DIR}"
        "${IRRKLANG_ROOT}/bin/winx64-visualStudio"
        "${IRRKLANG_ROOT}/bin/win32-visualStudio"
        "${IRRKLANG_ROOT}/bin"
        "${IRRKLANG_ROOT}/dll/${IRRKLANG_ARCH_DIR}"
        "${IRRKLANG_ROOT}/dll/winx64-visualStudio"
        "${IRRKLANG_ROOT}/dll/win32-visualStudio"
        "${IRRKLANG_ROOT}/dll/Winx64-visualStudio"
        "${IRRKLANG_ROOT}/dll/Win32-visualStudio"
        "${IRRKLANG_ROOT}/dll"
        "${IRRKLANG_ROOT}/lib/${IRRKLANG_ARCH_DIR}"
        "${IRRKLANG_ROOT}/lib"
    NO_DEFAULT_PATH
)

if(NOT IRRKLANG_INCLUDE_DIR OR NOT IRRKLANG_IMPLIB OR NOT IRRKLANG_RUNTIME)
    message(FATAL_ERROR "Could not locate irrKlang headers/libs/runtime under IRRKLANG_ROOT_DIR='${IRRKLANG_ROOT}'.")
endif()

file(INSTALL "${IRRKLANG_INCLUDE_DIR}/" DESTINATION "${CURRENT_PACKAGES_DIR}/include")

file(MAKE_DIRECTORY
    "${CURRENT_PACKAGES_DIR}/lib"
    "${CURRENT_PACKAGES_DIR}/bin"
    "${CURRENT_PACKAGES_DIR}/debug/lib"
    "${CURRENT_PACKAGES_DIR}/debug/bin"
    "${CURRENT_PACKAGES_DIR}/share/irrklang"
)

file(INSTALL "${IRRKLANG_IMPLIB}" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
file(INSTALL "${IRRKLANG_IMPLIB}" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
file(INSTALL "${IRRKLANG_RUNTIME}" DESTINATION "${CURRENT_PACKAGES_DIR}/bin")
file(INSTALL "${IRRKLANG_RUNTIME}" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin")

foreach(_plugin IN ITEMS ikpMP3.dll ikpFlac.dll)
    find_file(_IRRKLANG_PLUGIN
        NAMES "${_plugin}"
        PATHS
            "${IRRKLANG_ROOT}/bin/${IRRKLANG_ARCH_DIR}"
            "${IRRKLANG_ROOT}/bin/winx64-visualStudio"
            "${IRRKLANG_ROOT}/bin/win32-visualStudio"
            "${IRRKLANG_ROOT}/bin"
            "${IRRKLANG_ROOT}/dll/${IRRKLANG_ARCH_DIR}"
            "${IRRKLANG_ROOT}/dll/winx64-visualStudio"
            "${IRRKLANG_ROOT}/dll/win32-visualStudio"
            "${IRRKLANG_ROOT}/dll/Winx64-visualStudio"
            "${IRRKLANG_ROOT}/dll/Win32-visualStudio"
            "${IRRKLANG_ROOT}/dll"
        NO_DEFAULT_PATH
    )
    if(_IRRKLANG_PLUGIN)
        file(INSTALL "${_IRRKLANG_PLUGIN}" DESTINATION "${CURRENT_PACKAGES_DIR}/bin")
        file(INSTALL "${_IRRKLANG_PLUGIN}" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin")
    endif()
    unset(_IRRKLANG_PLUGIN CACHE)
endforeach()

set(IRRKLANG_CONFIG_CONTENT [=[
get_filename_component(_IRRKLANG_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

find_path(IRRKLANG_INCLUDE_DIR irrKlang.h PATHS "${_IRRKLANG_PREFIX}/include" NO_DEFAULT_PATH)
find_library(IRRKLANG_LIBRARY_RELEASE NAMES irrKlang irrklang PATHS "${_IRRKLANG_PREFIX}/lib" NO_DEFAULT_PATH)
find_library(IRRKLANG_LIBRARY_DEBUG NAMES irrKlang irrklang PATHS "${_IRRKLANG_PREFIX}/debug/lib" "${_IRRKLANG_PREFIX}/lib" NO_DEFAULT_PATH)
find_file(IRRKLANG_RUNTIME_LIBRARY_RELEASE NAMES irrKlang.dll PATHS "${_IRRKLANG_PREFIX}/bin" NO_DEFAULT_PATH)
find_file(IRRKLANG_RUNTIME_LIBRARY_DEBUG NAMES irrKlang.dll PATHS "${_IRRKLANG_PREFIX}/debug/bin" "${_IRRKLANG_PREFIX}/bin" NO_DEFAULT_PATH)

set(IrrKlang_FOUND FALSE)
set(irrklang_FOUND FALSE)
if(IRRKLANG_INCLUDE_DIR AND IRRKLANG_LIBRARY_RELEASE AND IRRKLANG_RUNTIME_LIBRARY_RELEASE)
    set(IrrKlang_FOUND TRUE)
    set(irrklang_FOUND TRUE)
    set(IRRKLANG_FOUND TRUE)
    set(IRRKLANG_INCLUDE_DIRS "${IRRKLANG_INCLUDE_DIR}")
    set(IRRKLANG_LIBRARIES "${IRRKLANG_LIBRARY_RELEASE}")
    set(IRRKLANG_RUNTIME_LIBRARY "${IRRKLANG_RUNTIME_LIBRARY_RELEASE}")
    set(IRRKLANG_PLUGIN_RUNTIME_LIBRARIES)
    foreach(_plugin IN ITEMS ikpMP3.dll ikpFlac.dll)
        string(MAKE_C_IDENTIFIER "${_plugin}" _plugin_var)
        find_file(_IRRKLANG_PLUGIN_${_plugin_var}
            NAMES "${_plugin}"
            PATHS "${_IRRKLANG_PREFIX}/bin" "${_IRRKLANG_PREFIX}/debug/bin"
            NO_DEFAULT_PATH
        )
        if(_IRRKLANG_PLUGIN_${_plugin_var})
            list(APPEND IRRKLANG_PLUGIN_RUNTIME_LIBRARIES "${_IRRKLANG_PLUGIN_${_plugin_var}}")
        endif()
    endforeach()

    if(NOT TARGET IrrKlang::IrrKlang)
        add_library(IrrKlang::IrrKlang SHARED IMPORTED)
        set_target_properties(IrrKlang::IrrKlang PROPERTIES
            IMPORTED_CONFIGURATIONS "DEBUG;RELEASE;RELWITHDEBINFO;MINSIZEREL"
            INTERFACE_INCLUDE_DIRECTORIES "${IRRKLANG_INCLUDE_DIR}"
            IMPORTED_IMPLIB "${IRRKLANG_LIBRARY_RELEASE}"
            IMPORTED_LOCATION "${IRRKLANG_RUNTIME_LIBRARY_RELEASE}"
            IMPORTED_IMPLIB_RELEASE "${IRRKLANG_LIBRARY_RELEASE}"
            IMPORTED_LOCATION_RELEASE "${IRRKLANG_RUNTIME_LIBRARY_RELEASE}"
            IMPORTED_IMPLIB_DEBUG "${IRRKLANG_LIBRARY_DEBUG}"
            IMPORTED_LOCATION_DEBUG "${IRRKLANG_RUNTIME_LIBRARY_DEBUG}"
            IMPORTED_IMPLIB_RELWITHDEBINFO "${IRRKLANG_LIBRARY_RELEASE}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${IRRKLANG_RUNTIME_LIBRARY_RELEASE}"
            IMPORTED_IMPLIB_MINSIZEREL "${IRRKLANG_LIBRARY_RELEASE}"
            IMPORTED_LOCATION_MINSIZEREL "${IRRKLANG_RUNTIME_LIBRARY_RELEASE}"
        )
    endif()
endif()
]=])
file(WRITE "${CURRENT_PACKAGES_DIR}/share/irrklang/irrklang-config.cmake" "${IRRKLANG_CONFIG_CONTENT}")
file(WRITE "${CURRENT_PACKAGES_DIR}/share/irrklang/IrrKlangConfig.cmake" "include(\"\${CMAKE_CURRENT_LIST_DIR}/irrklang-config.cmake\")\n")

if(EXISTS "${IRRKLANG_ROOT}/license.txt")
    file(INSTALL "${IRRKLANG_ROOT}/license.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/irrklang" RENAME copyright)
else()
    file(WRITE "${CURRENT_PACKAGES_DIR}/share/irrklang/copyright" "irrKlang SDK files are provided by the locally installed irrKlang SDK. Review your irrKlang license before redistribution.\n")
endif()
