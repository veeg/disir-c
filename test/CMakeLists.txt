
find_library (cmocka NAMES cmocka.a cmocka)

# Define a libdisir testsuite executable target
set (LIBDISIR_TESTSUITE_TARGET "libdisir_testsuite_executable" CACHE INTERNAL "testsuite executable target" FORCE)

# Testsuite executable sources
set (_LIBDISIR_TESTSUITE_SOURCES
    "libdisir_testsuite.c"
)

add_executable (${LIBDISIR_TESTSUITE_TARGET} ${_LIBDISIR_TESTSUITE_SOURCES})

# Include all the include directories for the testsuite.
include_directories (
    ${LIBDISIR_PUBLIC_INCLUDE_DIR}
    ${LIBDISIR_PRIVATE_INCLUDE_DIR}
    ${VOID-MULTIMAP_PUBLIC_INCLUDE_DIR}
    ${VOID-LIST_PUBLIC_INCLUDE_DIR}
)

add_definitions ("-Wall")
add_definitions ("-Werror")

target_link_libraries (${LIBDISIR_TESTSUITE_TARGET} ${PROJECT_SO_LIBRARY})
target_link_libraries (${LIBDISIR_TESTSUITE_TARGET} cmocka)
target_link_libraries (${LIBDISIR_TESTSUITE_TARGET} void-multimap)
target_link_libraries (${LIBDISIR_TESTSUITE_TARGET} void-list)

add_test (NAME LIBDisirTestsuite COMMAND ${LIBDISIR_TESTSUITE_TARGET})

