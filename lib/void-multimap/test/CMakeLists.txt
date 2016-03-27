
find_library (cmocka NAMES cmocka.a cmocka)

include_directories (${VOID-MULTIMAP_PUBLIC_INCLUDE_DIR})

set (MULTIMAP_TESTSUITE_SOURCES
    "multimap_testsuite.c"
)

add_executable (multimap_testsuite ${MULTIMAP_TESTSUITE_SOURCES})

add_definitions ("-Wall")
add_definitions ("-Werror")

target_link_libraries (multimap_testsuite void-multimap)
target_link_libraries (multimap_testsuite cmocka)

add_test (NAME MultimapTestsuite COMMAND multimap_testsuite)

