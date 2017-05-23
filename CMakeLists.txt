# Minimum required CMake version.
cmake_minimum_required (VERSION 3.2.0)

# Define the LibDisir project
project (disir VERSION 0.1.0 LANGUAGES C CXX)

include (CMakePackageConfigHelpers)
include (GNUInstallDirs)

add_subdirectory (lib)
add_subdirectory (plugins)
add_subdirectory (doc)
add_subdirectory (cli)

enable_testing ()
add_subdirectory (test)
