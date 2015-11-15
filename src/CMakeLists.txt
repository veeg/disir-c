
# Define all libdisir source files
set (_LIBDISIR_SOURCES
    "log.c"
    "util.c"
    "context.c"
    "context_util.c"
    "context_config.c"
    "context_documentation.c"
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

include_directories (
    ${LIBDISIR_PUBLIC_INCLUDE_DIR}
    ${LIBDISIR_PRIVATE_INCLUDE_DIR}
)

# Define a static libdisir library
add_library(${PROJECT_NAME} STATIC ${_LIBDISIR_SOURCES})

add_definitions ("-Wall")
add_definitions ("-Werror")

target_link_libraries(${PROJECT_NAME})

