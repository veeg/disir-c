# Minimum required CMake version.
cmake_minimum_required (VERSION 3.2.0)

# Define the LibDisir project
project (libdisir VERSION 0.1.0 LANGUAGES C CXX)

include (CMakePackageConfigHelpers)
include (GNUInstallDirs)

set (EXPORT_TARGET libdisir-targets)
set (EXPORT_NAMESPACE disir::)

add_subdirectory (lib)
add_subdirectory (plugin_test)
add_subdirectory (doc)


write_basic_package_version_file (
    "${CMAKE_CURRENT_BINARY_DIR}/libdisir/libdisir-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

export (
    EXPORT ${EXPORT_TARGET}
    FILE "${CMAKE_CURRENT_BINARY_DIR}/libdisir/libdisir-targets.cmake"
    NAMESPACE ${EXPORT_NAMESPACE}
)
configure_file (
    cmake/libdisir-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/libdisir/libdisir-config.cmake"
    COPYONLY
)

set (LIBDISIR_PACKAGE_LOCATION ${CMAKE_INSTALL_LIBDIR}/cmake/libdisir/)
install (
    EXPORT ${EXPORT_TARGET}
    FILE "${EXPORT_TARGET}.cmake"
    DESTINATION ${LIBDISIR_PACKAGE_LOCATION}
    NAMESPACE ${EXPORT_NAMESPACE}
)
install (
  FILES
    cmake/libdisir-config.cmake
    "${CMAKE_CURRENT_BINARY_DIR}/libdisir/libdisir-config-version.cmake"
  DESTINATION
    ${LIBDISIR_PACKAGE_LOCATION}
  COMPONENT
    Devel
)



enable_testing ()
add_subdirectory (test)
