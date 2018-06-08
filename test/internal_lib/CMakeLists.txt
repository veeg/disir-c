enable_testing()

set (TESTS_INTERNAL_LIB tests_internal_lib)
file (GLOB TESTS_INTERNAL_LIB_SOURCES *.cc)
list (APPEND TESTS_INTERNAL_LIB_SOURCES "../test_helper.cc" "../gtest.cc")

add_executable (${TESTS_INTERNAL_LIB} ${TESTS_INTERNAL_LIB_SOURCES})

include_directories (${LIBDISIR_TEST_INCLUDE_DIRS})

target_link_libraries (${TESTS_INTERNAL_LIB} ${PROJECT_STATIC_LIBRARY})
target_link_libraries (${TESTS_INTERNAL_LIB} ${GTEST_BOTH_LIBRARIES})
target_link_libraries (${TESTS_INTERNAL_LIB} ${CMAKE_DL_LIBS})
# TODO: Why pthread not part of gtest?? (it is on fedora)
target_link_libraries (${TESTS_INTERNAL_LIB} pthread)

add_test (LibDisirInternalLibTests ${TESTS_INTERNAL_LIB})

