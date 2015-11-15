

find_library (cmocka NAMES cmocka.a cmocka)

# Include all the inclulde directories in the disir source
include_directories(
    ${LIBDISIR_PUBLIC_INCLUDE_DIR}
    ${LIBDISIR_PRIVATE_INCLUDE_DIR}
)

# Testsuite executable sources
set (_LIBDISIR_TESTSUITE_SOURCES
    "libdisir_testsuite.c"
)

# Define a libdisir testsuite executable target
set (LIBDISIR_TESTSUITE_TARGET "libdisir_testsuite_executable" CACHE INTERNAL "testsuite executable target" FORCE)
add_executable (${LIBDISIR_TESTSUITE_TARGET} EXCLUDE_FROM_ALL ${_LIBDISIR_TESTSUITE_SOURCES})

add_test (NAME libdisir_testsuite COMMAND ${LIBDISIR_TESTSUITE_TARGET})
add_dependencies(build_and_test ${LIBDISIR_TESTSUITE_TARGET})

add_definitions ("-Wall")
add_definitions ("-Werror")

target_link_libraries(${LIBDISIR_TESTSUITE_TARGET} ${PROJECT_NAME})
target_link_libraries(${LIBDISIR_TESTSUITE_TARGET} cmocka)
