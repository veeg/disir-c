enable_testing()

set (TESTS_INTERNAL_UTIL tests_internal_util)
file (GLOB TESTS_INTERNAL_UTIL_SOURCES *.cc)
list (APPEND TESTS_INTERNAL_UTIL_SOURCES "../test_helper.cc")

add_executable (${TESTS_INTERNAL_UTIL} ${TESTS_INTERNAL_UTIL_SOURCES})

set_property (TARGET ${TESTS_INTERNAL_UTIL} PROPERTY CXX_STANDARD 11)
set_property (TARGET ${TESTS_INTERNAL_UTIL} PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions ("-Wall")
add_definitions ("-Werror")

include_directories (${LIBDISIR_TEST_INCLUDE_DIRS})

target_link_libraries (${TESTS_INTERNAL_UTIL} ${PROJECT_SO_LIBRARY})
target_link_libraries (${TESTS_INTERNAL_UTIL} ${GTEST_BOTH_LIBRARIES})
# TODO: Why pthread not part of gtest?? (it is on fedora)
target_link_libraries (${TESTS_INTERNAL_UTIL} pthread)
target_link_libraries (${TESTS_INTERNAL_UTIL} void-multimap)
target_link_libraries (${TESTS_INTERNAL_UTIL} void-list)

add_test (LibDisirInternalUtilTests ${TESTS_INTERNAL_UTIL})

