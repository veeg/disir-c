
set (VOID-MULTIMAP_PUBLIC_INCLUDE_DIR 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    CACHE INTERNAL 
    "${PROJECT_NAME} public include directory." 
    FORCE 
)

add_library (void-multimap multimap.c)
add_definitions ("-Wall")
add_definitions ("-Werror")
