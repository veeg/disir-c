

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
    "context_documentation.c"
    "collection.c"
    "element_storage.c"
    "disir.c"
    "io.c"
    "io/ini.c"
    "update.c"
    "${CMAKE_CURRENT_BINARY_DIR}/version.c"
)

# Export public include directory as internal cache variable
set (LIBDISIR_PUBLIC_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/public_include"
    CACHE INTERNAL "${PROJECT_NAME}: Public include directory" FORCE
)

# Export the private include directory as internal cache variable
set (LIBDISIR_PRIVATE_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/private_include"
    CACHE INTERNAL "${PROJECT_NAME}: Private include directory" FORCE
)


set (PROJECT_SO_LIBRARY disir)
# Define shared library
add_library(${PROJECT_SO_LIBRARY} SHARED ${_LIBDISIR_SOURCES})
add_dependencies (${PROJECT_SO_LIBRARY} configure_version)

include_directories (
    ${LIBDISIR_PUBLIC_INCLUDE_DIR}
    ${LIBDISIR_PRIVATE_INCLUDE_DIR}
    ${VOID-MULTIMAP_PUBLIC_INCLUDE_DIR}
    ${VOID-LIST_PUBLIC_INCLUDE_DIR}
)

set_target_properties (${PROJECT_SO_LIBRARY} PROPERTIES COMPILE_FLAGS "-fPIC")

add_definitions ("-Wall")
add_definitions ("-Werror")

target_link_libraries (${PROJECT_SO_LIBRARY} void-multimap)
target_link_libraries (${PROJECT_SO_LIBRARY} void-list)

install (TARGETS ${PROJECT_SO_LIBRARY}
    EXPORT ${EXPORT_TARGET}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    INCLUDES DESTINATION include
)

install (
  DIRECTORY ${LIBDISIR_PUBLIC_INCLUDE_DIR}/
  DESTINATION include
  COMPONENT Devel
)
