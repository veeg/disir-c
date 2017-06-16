enable_testing()

set (TESTS_PLUGIN_TOML tests_plugin_toml)
file (GLOB TESTS_PLUGIN_TOML_SOURCES *.cc)
list (APPEND TESTS_PLUGIN_TOML_SOURCES "../../test_helper.cc" "../../gtest.cc")

add_executable (${TESTS_PLUGIN_TOML} ${TESTS_PLUGIN_TOML_SOURCES})

include_directories (${LIBDISIR_TEST_INCLUDE_DIRS})
include_directories (${CMAKE_SOURCE_DIR}/3rdparty/)
include_directories (${CMAKE_SOURCE_DIR}/3rdparty/boost_fdstream)
include_directories (${CMAKE_SOURCE_DIR}/include/)

target_link_libraries (${TESTS_PLUGIN_TOML} ${PROJECT_SO_LIBRARY})
target_link_libraries (${TESTS_PLUGIN_TOML} ${GTEST_BOTH_LIBRARIES})
# TODO: Why pthread not part of gtest?? (it is on fedora)
target_link_libraries (${TESTS_PLUGIN_TOML} pthread)

add_test (LibDisirPluginTOMLTest ${TESTS_PLUGIN_TOML})

