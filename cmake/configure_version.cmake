
string (RANDOM LENGTH 8 ALPHABET 0123456789 build)
math (EXPR number "${build} + 0") # Remove extra leading 0's

configure_file (${SRC_DIR}/version.c.in ${BIN_DIR}/version.c @ONLY)

message (STATUS "version ${PROJECT_VERSION} build ${build}")

