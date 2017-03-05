
set (JSONCPP_ROOTDIR ${PROJECT_SOURCE_DIR}/lib/jsoncpp)

set (PLUGIN_JSON_SOURCES
  "${JSONCPP_ROOTDIR}/jsoncpp.cpp"
  "main.cc"
  "input.cc"
  "output.cc"
  "log.cc"
  "jsonIO.cc"
  "marshal_config.cc"
  "marshal_mold.cc"
  "unmarshal_config.cc"
  "unmarshal_mold.cc"
  "util.cc"
)

set (JSON_TARGET dplugin_json)

add_library (${JSON_TARGET} SHARED ${PLUGIN_JSON_SOURCES})
set_property (TARGET ${JSON_TARGET} PROPERTY CXX_STANDARD 11)
set_property (TARGET ${JSON_TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property (TARGET ${JSON_TARGET} PROPERTY PREFIX "")


add_definitions ("-Wall")
add_definitions ("-Werror")
add_definitions ("-g")

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${JSONCPP_ROOTDIR}
)

target_link_libraries (${JSON_TARGET} disir::disir)


install (
  TARGETS ${JSON_TARGET}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/disir/plugins/
)

