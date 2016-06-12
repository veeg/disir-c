enable_testing()

find_package(GTest REQUIRED)

set (LIBDISIR_TEST_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${GTEST_INCLUDE_DIRS}
  # Hack it. Should find a better way to specify libdisir private include dir
  ${CMAKE_SOURCE_DIR}/src/private_include/
  ${VOID-MULTIMAP_PUBLIC_INCLUDE_DIR}
  ${VOID-LIST_PUBLIC_INCLUDE_DIR}
  CACHE INTERNAL "libdisir: test include directories" FORCE
)

add_subdirectory (public_api)
add_subdirectory (internal_lib)
add_subdirectory (internal_util)

