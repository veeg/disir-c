
set (PLUGIN_TEST_SOURCES
    "main.cc"
)

set (TEST_TARGET dplugin_test)

add_library (${TEST_TARGET} SHARED ${PLUGIN_TEST_SOURCES})
set_property (TARGET ${TEST_TARGET} PROPERTY CXX_STANDARD 11)
set_property (TARGET ${TEST_TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property (TARGET ${TEST_TARGET} PROPERTY PREFIX "")

add_definitions ("-Wall")
add_definitions ("-Werror")

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries (${TEST_TARGET} void-multimap)
target_link_libraries (${TEST_TARGET} void-list)
target_link_libraries (${TEST_TARGET} ${PROJET_SO_LIBRARY})


install (
  TARGETS ${TEST_TARGET}
  LIBRARY DESTINATION lib/disir/plugins/
)
