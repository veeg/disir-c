enable_testing()

set (TESTS_PLUGIN_JSON tests_plugin_json)
file (GLOB TESTS_PLUGIN_JSON_SOURCES *.cc)
list (APPEND TESTS_PLUGIN_JSON_SOURCES "../../test_helper.cc" "../../gtest.cc")
list (APPEND TESTS_PLUGIN_JSON_SOURCES ${CMAKE_SOURCE_DIR}/lib/log.c)
list (APPEND TESTS_PLUGIN_JSON_SOURCES ${CMAKE_SOURCE_DIR}/3rdparty/jsoncpp/jsoncpp.cpp)
file (GLOB json_libsources ${CMAKE_SOURCE_DIR}/lib/fslib/json/*.cc)
list (APPEND TESTS_PLUGIN_JSON_SOURCES ${json_libsources})


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

# Only set the implict-fallthrough lint on GCC >= 7
if (CMAKE_COMPILER_IS_GNUCC)
  if (NOT (CMAKE_C_COMPILER_VERSION LESS "7"))
    set_target_properties (${TESTS_PLUGIN_JSON}
      PROPERTIES COMPILE_FLAGS -Wimplicit-fallthrough=0)
  endif()
endif()

add_test (LibDisirPluginJSONTest ${TESTS_PLUGIN_JSON})

