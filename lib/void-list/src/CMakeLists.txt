
set (LIST_PUBLIC_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    CACHE INTERNAL
    "${PROJECT_NAME} public include directory."
    FORCE
)

set (LIST_SOURCES
    "list.c"
    "list_iterator.c"
    "list_node.c"
)

add_library (${PROJECT_NAME} STATIC ${LIST_SOURCES})

include_directories (${LIST_PUBLIC_INCLUDE_DIR})

add_definitions ("-Wall")
add_definitions ("-Werror")


