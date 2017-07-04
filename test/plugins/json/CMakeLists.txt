enable_testing()

set (TESTS_PLUGIN_JSON tests_plugin_json)
file (GLOB TESTS_PLUGIN_JSON_SOURCES *.cc)
list (APPEND TESTS_PLUGIN_JSON_SOURCES "../../test_helper.cc" "../../gtest.cc")

add_executable (${TESTS_PLUGIN_JSON} ${TESTS_PLUGIN_JSON_SOURCES})

include_directories (${LIBDISIR_TEST_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/3rdparty/)
include_directories (${CMAKE_SOURCE_DIR}/3rdparty/boost_fdstream)
include_directories (${CMAKE_SOURCE_DIR}/3rdparty/jsoncpp)
include_directories (${CMAKE_SOURCE_DIR}/include/)
# Private include headers
include_directories (${CMAKE_SOURCE_DIR}/lib/include/)

target_link_libraries (${TESTS_PLUGIN_JSON} ${PROJECT_SO_LIBRARY})
target_link_libraries (${TESTS_PLUGIN_JSON} ${GTEST_BOTH_LIBRARIES})
# TODO: Why pthread not part of gtest?? (it is on fedora)
target_link_libraries (${TESTS_PLUGIN_JSON} pthread)
# cpp experimental filesystem
target_link_libraries (${TESTS_PLUGIN_JSON} stdc++fs)

add_test (LibDisirPluginJSONTest ${TESTS_PLUGIN_JSON})

