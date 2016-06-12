
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

target_link_libraries (${TEST_TARGET} ${PROJECT_SO_LIBRARY})

install (
  TARGETS ${TEST_TARGET}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/disir/plugins/
)
