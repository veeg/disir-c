enable_testing()

find_package(MEOSGTest REQUIRED)

configure_file (test_helper.h.in test_helper.h)

set (LIBDISIR_TEST_INCLUDE_DIRS
  ${CMAKE_CURRENT_BINARY_DIR}
  ${GTEST_INCLUDE_DIRS}
  # Hack it. Should find a better way to specify libdisir private include dir
  ${CMAKE_SOURCE_DIR}/lib/include/
  ${VOID-MULTIMAP_PUBLIC_INCLUDE_DIR}
  ${VOID-LIST_PUBLIC_INCLUDE_DIR}
  CACHE INTERNAL "libdisir: test include directories" FORCE
)

# We cannot be pedantic about internal test code.
# There are a few instances where mixing C definitions
# and C++ test code doesnt go too well with the pedantic flags
# Also, gtest doesnt like the vararic parameterized macro in ISO c++11
remove_definitions (-pedantic)

add_subdirectory (public_api)
add_subdirectory (internal_lib)
add_subdirectory (internal_util)
add_subdirectory (plugins)
