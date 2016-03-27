
set (VOID-LIST_PUBLIC_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    CACHE INTERNAL
    "${PROJECT_NAME} public include directory."
    FORCE
)

set (VOID-LIST_PRIVATE_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL
    "${PROJECT_NAME} private include directory."
    FORCE
)

set (LIST_SOURCES
    "list.c"
    "list_iterator.c"
)

add_library (${PROJECT_NAME} STATIC ${LIST_SOURCES})

include_directories (${VOID-LIST_PUBLIC_INCLUDE_DIR})

add_definitions ("-Wall")
add_definitions ("-Werror")

install (
    TARGETS ${PROJECT_NAME}
    EXPORT ${EXPORT_TARGET}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

