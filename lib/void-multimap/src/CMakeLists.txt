
set (VOID-MULTIMAP_PUBLIC_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL
    "${PROJECT_NAME} public include directory."
    FORCE
)

set (MULTIMAP_SOURCES
    "multimap.c"
)

add_library (${PROJECT_NAME} STATIC ${MULTIMAP_SOURCES})

include_directories (${VOID-MULTIMAP_PUBLIC_INCLUDE_DIR})

add_definitions ("-Wall")
add_definitions ("-Werror")
