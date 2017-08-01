# Minimum required CMake version.
cmake_minimum_required (VERSION 3.2.0)

# Define the LibDisir project
project (disir VERSION 0.1.0 LANGUAGES C CXX)

include (CMakePackageConfigHelpers)
include (GNUInstallDirs)

set (CMAKE_C_STANDARD 11)
set (CMAKE_CXSTANDARD_REQUIRED ON)
set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Set generic compiler flags applicable to both C and C++
add_definitions (-pedantic)
add_definitions (-Werror)
add_definitions (-Wall)
add_definitions (-Wextra)
add_definitions (-Wcast-qual)
add_definitions (-Wcast-align)
add_definitions (-Wvla)
add_definitions (-Wstack-protector)
add_definitions (-Wformat-security)
add_definitions (-Wlogical-op)

# Set C only compile flags
set_property (GLOBAL APPEND PROPERTY COMPILE_FLAGS -Wmissing-prototypes -Wwrite-strings)
# GCC > 4.8
add_definitions (-Werror=return-local-addr)
# GCC > 4.9
add_definitions (-Wdate-time)

# TMP: MEOS
set (CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} /usr/share/meos-pkgtools/cmake)

add_subdirectory (lib)
add_subdirectory (plugins)
add_subdirectory (cli)
add_subdirectory (doc)

enable_testing ()
add_subdirectory (test)
