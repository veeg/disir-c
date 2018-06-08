enable_testing()

set (TESTS_INTERNAL_UTIL tests_internal_util)
file (GLOB TESTS_INTERNAL_UTIL_SOURCES *.cc)
list (APPEND TESTS_INTERNAL_UTIL_SOURCES "../test_helper.cc" "../gtest.cc")
list (APPEND TESTS_INTERNAL_UTIL_SOURCES ${CMAKE_SOURCE_DIR}/lib/log.c)

add_executable (${TESTS_INTERNAL_UTIL} ${TESTS_INTERNAL_UTIL_SOURCES})

include_directories (${LIBDISIR_TEST_INCLUDE_DIRS})

target_link_libraries (${TESTS_INTERNAL_UTIL} ${PROJECT_STATIC_LIBRARY})
target_link_libraries (${TESTS_INTERNAL_UTIL} ${GTEST_BOTH_LIBRARIES})
target_link_libraries (${TESTS_INTERNAL_UTIL} ${CMAKE_DL_LIBS})
#target_link_libraries (${TESTS_INTERNAL_UTIL} ${PROJECT_TESTSRC_LIBRARY})

# TODO: Why pthread not part of gtest?? (it is on fedora)
target_link_libraries (${TESTS_INTERNAL_UTIL} pthread)

add_test (LibDisirInternalUtilTests ${TESTS_INTERNAL_UTIL})

