enable_testing()

set (TESTS_INTERNAL_UTIL tests_internal_util)
file (GLOB TESTS_INTERNAL_UTIL_SOURCES *.cc)
list (APPEND TESTS_INTERNAL_UTIL_SOURCES "../test_helper.cc")

add_executable (${TESTS_INTERNAL_UTIL} ${TESTS_INTERNAL_UTIL_SOURCES})

include_directories (${LIBDISIR_TEST_INCLUDE_DIRS})

target_link_libraries (${TESTS_INTERNAL_UTIL} ${PROJECT_SO_LIBRARY})
target_link_libraries (${TESTS_INTERNAL_UTIL} ${GTEST_BOTH_LIBRARIES})
# TODO: Why pthread not part of gtest?? (it is on fedora)
target_link_libraries (${TESTS_INTERNAL_UTIL} pthread)

add_test (LibDisirInternalUtilTests ${TESTS_INTERNAL_UTIL})

