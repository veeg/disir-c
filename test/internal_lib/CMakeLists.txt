enable_testing()

set (TESTS_INTERNAL_LIB tests_internal_lib)
file (GLOB TESTS_INTERNAL_LIB_SOURCES *.cc)
list (APPEND TESTS_INTERNAL_LIB_SOURCES "../test_helper.cc")

add_executable (${TESTS_INTERNAL_LIB} ${TESTS_INTERNAL_LIB_SOURCES})

set_property (TARGET ${TESTS_INTERNAL_LIB} PROPERTY CXX_STANDARD 11)
set_property (TARGET ${TESTS_INTERNAL_LIB} PROPERTY CXX_STANDARD_REQUIRED ON)

add_definitions ("-Wall")
add_definitions ("-Werror")

include_directories (${LIBDISIR_TEST_INCLUDE_DIRS})

target_link_libraries (${TESTS_INTERNAL_LIB} ${PROJECT_SO_LIBRARY})
target_link_libraries (${TESTS_INTERNAL_LIB} ${GTEST_BOTH_LIBRARIES})
# TODO: Why pthread not part of gtest?? (it is on fedora)
target_link_libraries (${TESTS_INTERNAL_LIB} pthread)
target_link_libraries (${TESTS_INTERNAL_LIB} void-multimap)
target_link_libraries (${TESTS_INTERNAL_LIB} void-list)

add_test (LibDisirInternalLibTests ${TESTS_INTERNAL_LIB})

