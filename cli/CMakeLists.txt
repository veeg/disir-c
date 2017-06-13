
set (CLIDISIR_SOURCES
    "main.cc"
    "cli.cc"
    "command.cc"
    "command_list.cc"
    "command_generate.cc"
    "command_verify.cc"
)

set (CLI_TARGET cli)
add_executable (${CLI_TARGET} ${CLIDISIR_SOURCES})
set_property (TARGET ${CLI_TARGET} PROPERTY CXX_STANDARD 11)
set_property (TARGET ${CLI_TARGET} PROPERTY CXX_STANDARD_REQUIRED ON)
set_property (TARGET ${CLI_TARGET} PROPERTY OUTPUT_NAME disir)

add_definitions ("-Wall")
add_definitions ("-Werror")

include_directories (
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries (${CLI_TARGET} ${PROJECT_SO_LIBRARY})

install (
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/disir
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

