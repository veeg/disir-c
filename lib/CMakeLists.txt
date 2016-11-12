
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
)

set (_LIBDISIR_3PARTY_LIB_INCLUDEDIR
  ${PROJECT_3PARTY_LIBDIR}/void-multimap
  ${PROJECT_3PARTY_LIBDIR}/void-list/include
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
    "context_free_text.c"
    "context_restriction.c"
    "collection.c"
    "element_storage.c"
    "disir.c"
    "disir_config.c"
    "disir_entry.c"
    "disir_mold.c"
    "disir_plugin.c"
    "io/ini.c"
    "update.c"
    "validate.c"
    "${CMAKE_CURRENT_BINARY_DIR}/version.c"
    ${_LIBDISIR_3PARTY_LIB_SOURCES}
)

set (PROJECT_SO_LIBRARY disir
    CACHE INTERNAL "${PROJECT_NAME}: shared library" FORCE)
# Define shared library
add_library(${PROJECT_SO_LIBRARY} SHARED ${_LIBDISIR_SOURCES})
add_dependencies (${PROJECT_SO_LIBRARY} configure_version)

target_include_directories (${PROJECT_SO_LIBRARY} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include/
  ${_LIBDISIR_3PARTY_LIB_INCLUDEDIR}
)

target_include_directories (${PROJECT_SO_LIBRARY} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/>
  $<INSTALL_INTERFACE:include/>
)

set_target_properties (${PROJECT_SO_LIBRARY} PROPERTIES COMPILE_FLAGS "-fPIC")

add_definitions ("-Wall")
add_definitions ("-Werror")
# TODO: Add check for gcc > 4.9
#add_definitions ("-fsanitize=undefined")
#add_definitions ("-fsanitize=shift")
#add_definitions ("-fsanitize=integer-divide-by-zero")
#add_definitions ("-fsanitize=unreachable")
#add_definitions ("-fsanitize=vla-bound")
#add_definitions ("-fsanitize=null")
#add_definitions ("-fsanitize=return")
#add_definitions ("-fsanitize=signed-integer-overflow")

# We require DL_LIBS for your loading plugin functionality.
target_link_libraries (${PROJECT_SO_LIBRARY} ${CMAKE_DL_LIBS})
# TODO: Add check for gcc > 4.9
#target_link_libraries (${PROJECT_SO_LIBRARY} ubsan)

install (TARGETS ${PROJECT_SO_LIBRARY}
    EXPORT ${EXPORT_TARGET}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION include
)

install (
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/public_include/
  DESTINATION include
  COMPONENT Devel
)