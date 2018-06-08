
set (build "ini")
configure_file (version.c.in ${CMAKE_CURRENT_BINARY_DIR}/version.c)
add_custom_target (configure_version ALL
  COMMAND ${CMAKE_COMMAND}
    -D SRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}
    -D BIN_DIR=${CMAKE_CURRENT_BINARY_DIR}
    -D PROJECT_VERSION=${PROJECT_VERSION}
    -D PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    -D PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    -D PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    -P ${CMAKE_SOURCE_DIR}/cmake/configure_version.cmake
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/version.c.in
  COMMENT "Configuring version.c"
)

set (PROJECT_3PARTY_LIBDIR ${CMAKE_SOURCE_DIR}/3rdparty)
set (_LIBDISIR_3PARTY_LIB_SOURCES
  "${PROJECT_3PARTY_LIBDIR}/void-multimap/multimap.c"
  "${PROJECT_3PARTY_LIBDIR}/void-list/list.c"
  "${PROJECT_3PARTY_LIBDIR}/void-list/list_iterator.c"
  "${PROJECT_3PARTY_LIBDIR}/jsoncpp/jsoncpp.cpp"
)

set (_LIBDISIR_3PARTY_LIB_INCLUDEDIR
  ${PROJECT_3PARTY_LIBDIR}/void-multimap
  ${PROJECT_3PARTY_LIBDIR}/void-list/include
  ${PROJECT_3PARTY_LIBDIR}/boost_fdstream
  ${PROJECT_3PARTY_LIBDIR}/jsoncpp
  ${PROJECT_3PARTY_LIBDIR}
)

set (FSLIB_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/toml.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/toml/toml_serialize.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/toml/toml_unserialize.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/mkdir.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/namespace.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/filepath.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/query.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/read.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/write.c"


  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/json.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/json/json_serialize.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/json/json_unserialize.cc"

  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/json/jsonIO.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/json/json_serialize_config.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/json/json_unserialize_config.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/json/json_serialize_mold.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/json/json_unserialize_mold.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/fslib/json/json_mold_namespace_override.cc"

)

# Define all libdisir source files
set (_LIBDISIR_SOURCES
    "log.c"
    "util.c"
    "context.c"
    "context_default.c"
    "context_keyval.c"
    "context_mold.c"
    "context_util.c"
    "context_config.c"
    "context_section.c"
    "context_documentation.c"
    "context_value.c"
    "context_restriction.c"
    "collection.c"
    "element_storage.c"
    "error.c"
    "disir.c"
    "disir_archive.cc"
    "disir_archive_util.cc"
    "disir_export.cc"
    "disir_config.c"
    "disir_import.c"
    "disir_config_query.c"
    "disir_entry.c"
    "disir_mold.c"
    "disir_plugin.c"
    "generate.c"
    "instance_mold.c"
    "update.c"
    "validate.c"
    "compare.c"
    "query.c"
    "${CMAKE_CURRENT_BINARY_DIR}/version.c"
    ${_LIBDISIR_3PARTY_LIB_SOURCES}
    ${FSLIB_SOURCES}
    test/plugin.cc
)

# Create an object library out of our sources
set (PROJECT_OBJ_LIBRARY libdisir_objlib)

add_library (libdisir_objlib OBJECT ${_LIBDISIR_SOURCES})
add_dependencies (${PROJECT_OBJ_LIBRARY} configure_version)

set_target_properties (${PROJECT_OBJ_LIBRARY} PROPERTIES POSITION_INDEPENDENT_CODE ON)
set_target_properties (${PROJECT_OBJ_LIBRARY} PROPERTIES
  C_VISIBILITY_PRESET hidden
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON)

# Include properties for this object library
target_include_directories (${PROJECT_OBJ_LIBRARY} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/
  ${_LIBDISIR_3PARTY_LIB_INCLUDEDIR}
)
target_include_directories (${PROJECT_OBJ_LIBRARY} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/>
  $<INSTALL_INTERFACE:include/>
)

# Define the other libraries
set (PROJECT_SO_LIBRARY disir_c
    CACHE INTERNAL "${PROJECT_NAME}: shared library" FORCE)
set (PROJECT_STATIC_LIBRARY disir_static
    CACHE INTERNAL "${PROJECT_NAME}: static library" FORCE)

# Define shared library
add_library(${PROJECT_SO_LIBRARY} SHARED $<TARGET_OBJECTS:libdisir_objlib>)
# Define static library - we do not want to export this, only for internal test usage.
add_library(${PROJECT_STATIC_LIBRARY} STATIC $<TARGET_OBJECTS:libdisir_objlib>)

set_target_properties (${PROJECT_SO_LIBRARY} PROPERTIES SOVERSION 0)

# Only set the implict-fallthrough lint on GCC >= 7
if (CMAKE_COMPILER_IS_GNUCC)
  if (NOT (CMAKE_C_COMPILER_VERSION LESS "7"))
    set_target_properties (${PROJECT_OBJ_LIBRARY}
      PROPERTIES COMPILE_FLAGS -Wimplicit-fallthrough=0)
  endif()
endif()

# We require DL_LIBS for your loading plugin functionality.
target_link_libraries (${PROJECT_SO_LIBRARY} ${CMAKE_DL_LIBS})
target_link_libraries (${PROJECT_SO_LIBRARY} ${ARCHIVE_LIBRARIES})

install (TARGETS ${PROJECT_SO_LIBRARY}
    EXPORT ${EXPORT_TARGET}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION include
)

install (
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/
  DESTINATION include
  COMPONENT Devel
)
