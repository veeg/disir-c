
set (PLUGIN_TOML_SOURCES
    "toml.cc"
)

set (TOML_TARGET dplugin_toml)

add_library (${TOML_TARGET} SHARED ${PLUGIN_TOML_SOURCES})

set_property (TARGET ${TOML_TARGET} PROPERTY CXX_STANDARD 11)
set_property (TARGET ${TOML_TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property (TARGET ${TOML_TARGET} PROPERTY PREFIX "")

add_definitions ("-Wall")
add_definitions ("-Werror")

target_link_libraries (${TOML_TARGET} ${PROJECT_SO_LIBRARY})

install (
  TARGETS ${TOML_TARGET}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/disir/plugins/
)
