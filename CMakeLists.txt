# Minimum required CMake version.
cmake_minimum_required (VERSION 3.2.0)

# Define the LibDisir project
project (libdisir VERSION 0.1.0 LANGUAGES C)

# Include the source directory
add_subdirectory (src)

# Enable testing, define custom 'build_and_test' target and finally include the test directory
# This setup enables the target build_and_test to be called separately from the tests, 
# which will be build and ran when the target 'build_and_test' is ran, e.g., 'make build_and_test'
enable_testing ()
add_custom_target (build_and_test ${CMAKE_CTEST_COMMAND} -V)
add_subdirectory (test)
