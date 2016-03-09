
add_executable (list_testsuite test.c)

add_definitions ("-Wall")
add_definitions ("-Werror")

include_directories (${LIST_PUBLIC_INCLUDE_DIR})

target_link_libraries (list_testsuite ${PROJECT_NAME})

add_test (ListTestsuite list_testsuite)

