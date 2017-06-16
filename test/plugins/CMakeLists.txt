enable_testing ()

add_subdirectory (toml)
add_subdirectory (json)

set (TESTS_PLUGIN_PLUGIN tests_plugin_plugin)
file (GLOB TESTS_PLUGIN_PLUGIN_SOURCES *.cc)
list (APPEND TESTS_PLUGIN_PLUGIN_SOURCES "../test_helper.cc")

add_executable (${TESTS_PLUGIN_PLUGIN} ${TESTS_PLUGIN_PLUGIN_SOURCES})

include_directories (${LIBDISIR_TEST_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/3rdparty/)
include_directories (${CMAKE_SOURCE_DIR}/3rdparty/boost_fdstream)
include_directories (${CMAKE_SOURCE_DIR}/include/)

target_link_libraries (${TESTS_PLUGIN_PLUGIN} ${PROJECT_SO_LIBRARY})
target_link_libraries (${TESTS_PLUGIN_PLUGIN} ${GTEST_BOTH_LIBRARIES})
# TODO: Why pthread not part of gtest?? (it is on fedora)
target_link_libraries (${TESTS_PLUGIN_PLUGIN} pthread)

add_test (LibDisirPluginTest ${TESTS_PLUGIN_PLUGIN})

