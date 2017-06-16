enable_testing()

set (TESTS_PUBLIC_API tests_public_api)
file (GLOB TESTS_PUBLIC_API_SOURCES *.cc)
list (APPEND TESTS_PUBLIC_API_SOURCES "../test_helper.cc" "../gtest.cc")

add_executable (${TESTS_PUBLIC_API} ${TESTS_PUBLIC_API_SOURCES})

include_directories (${LIBDISIR_TEST_INCLUDE_DIRS})

target_link_libraries (${TESTS_PUBLIC_API} ${PROJECT_SO_LIBRARY})
target_link_libraries (${TESTS_PUBLIC_API} ${GTEST_BOTH_LIBRARIES})
# TODO: Why pthread not part of gtest?? (it is on fedora)
target_link_libraries (${TESTS_PUBLIC_API} pthread)

add_test (LibDisirPublicAPITests ${TESTS_PUBLIC_API})

