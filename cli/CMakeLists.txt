
set (CLIDISIR_SOURCES
    "main.cc"
    "cli.cc"
    "command.cc"
    "command_list.cc"
    "command_generate.cc"
    "command_verify.cc"
    "command_dump.cc"
    "command_export.cc"
    "command_import.cc"
)

set (CLI_TARGET cli)
add_executable (${CLI_TARGET} ${CLIDISIR_SOURCES})
set_property (TARGET ${CLI_TARGET} PROPERTY OUTPUT_NAME disir)

include_directories (
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries (${CLI_TARGET} ${PROJECT_SO_LIBRARY})

install (
  PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/disir
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

