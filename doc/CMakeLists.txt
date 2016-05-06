
find_program(ENV_EXECUTABLE env)
find_program(RUBY_EXECUTABLE ruby)
find_program(DOXML2ASCIIDOC_EXECUTABLE doxml2asciidoc)
find_program(ASCIIDOCTOR_EXECUTABLE asciidoctor)

find_package(Doxygen)

# Require doxygen
if (NOT DOXYGEN_FOUND)
  add_custom_target (doc
    ALL DEPENDS
    COMMAND ${CMAKE_COMMAND} -E echo
      "WARNING: Building documentation requires doxygen. Please install it."
    COMMAND ${CMAKE_COMMAND} -E echo "No documentation will be build nor installed."
  )
  return()
endif(NOT DOXYGEN_FOUND)

# Require Ruby
if (${RUBY_EXECUTABLE} STREQUAL RUBY_EXECUTABLE-NOTFOUND)
  add_custom_target (doc
    ALL DEPENDS
    COMMAND ${CMAKE_COMMAND} -E echo
      "WARNING: Building documentation requires the Ruby runtime. Please install it."
    COMMAND ${CMAKE_COMMAND} -E echo "No documentation will be build nor installed."
  )
  return()
endif()

# Require DoXML2AsciiDoc
if (${DOXML2ASCIIDOC_EXECUTABLE} STREQUAL DOXML2ASCIIDOC_EXECUTABLE-NOTFOUND)
  add_custom_target (doc
    ALL DEPENDS
    COMMAND ${CMAKE_COMMAND} -E echo
      "WARNING: Cannot build documentation - missing doxml2asciidoc\n"
    COMMAND ${CMAKE_COMMAND} -E echo
      "Checkout www.github.com/veeg/doxml2asciidoc for the ruby gem."
    COMMAND ${CMAKE_COMMAND} -E echo "No documentation will be build nor installed."
  )
  return()
endif()

# Require Asciidoctor
if (${ASCIIDOCTOR_EXECUTABLE} STREQUAL ASCIIDOCTOR_EXECUTABLE-NOTFOUND)
  add_custom_target (doc
    ALL DEPENDS
    COMMAND ${CMAKE_COMMAND} -E echo
      "WARNING: Cannot build documentation - missing asciidoctor\n"
    COMMAND ${CMAKE_COMMAND} -E echo
      "To acquire the ruby gem, type: gem install asciidoctor"
    COMMAND ${CMAKE_COMMAND} -E echo "No documentation will be build nor installed."
  )
  return()
endif()


# Environment variable controls output directory for doxygen
set (
    DOXYGEN_OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/doxygen_output"
)

# Environment variable to control input directory for doxygen
set (
    DOXYGEN_INPUT_DIR
    "${CMAKE_SOURCE_DIR}/src/public_include/disir"
)

# Collect all generated and fixed documentation in a binary output folder
set (DOCUMENTATION_BINARY_ROOTDIR ${CMAKE_BINARY_DIR}/documentation)
file (MAKE_DIRECTORY ${DOCUMENTATION_BINARY_ROOTDIR})

# Make a new dedicated target to build public interface documentation
set (DOC_PUBLIC_API_FILENAME libdisir_public_api)
add_custom_target (doc
  ALL
  DEPENDS ${DOCUMENTATION_BINARY_ROOTDIR}/${DOC_PUBLIC_API_FILENAME}.html
)

# Generate the index.xml
add_custom_command (
  OUTPUT "${DOXYGEN_OUTPUT_DIR}/xml/index.xml"
  COMMAND ${ENV_EXECUTABLE} DOXYGEN_OUTPUT_DIR=${DOXYGEN_OUTPUT_DIR}
                            DOXYGEN_INPUT_DIR=${DOXYGEN_INPUT_DIR}
          doxygen ${CMAKE_CURRENT_SOURCE_DIR}/libdisir_public_documentation_doxygen.conf
                  > /dev/null
  COMMENT "Doxygen - building XML"
)

add_custom_command (
  OUTPUT "${DOXYGEN_OUTPUT_DIR}/xml/index.adoc"
  DEPENDS "${DOXYGEN_OUTPUT_DIR}/xml/index.xml"
  COMMAND ${DOXML2ASCIIDOC_EXECUTABLE} ${DOXYGEN_OUTPUT_DIR}/xml/index.xml > /dev/null
  WORKING_DIRECTORY ${DOXYGEN_OUTPUT_DIR}/xml
  COMMENT "DoXML2AsciiDoc - converting index.xml to index.adoc"
)

# Rename output from doxml2asciidoc
add_custom_command (
  DEPENDS ${DOXYGEN_OUTPUT_DIR}/xml/index.adoc
  OUTPUT ${DOCUMENTATION_BINARY_ROOTDIR}/${DOC_PUBLIC_API_FILENAME}.adoc
  COMMAND ${CMAKE_COMMAND} -E copy  ${DOXYGEN_OUTPUT_DIR}/xml/index.adoc
                                    ${DOCUMENTATION_BINARY_ROOTDIR}/${DOC_PUBLIC_API_FILENAME}.adoc
)

# Run asciidoctor on the output
add_custom_command (
  DEPENDS ${DOCUMENTATION_BINARY_ROOTDIR}/${DOC_PUBLIC_API_FILENAME}.adoc
  OUTPUT ${DOCUMENTATION_BINARY_ROOTDIR}/${DOC_PUBLIC_API_FILENAME}.html
  COMMAND ${ASCIIDOCTOR_EXECUTABLE} ${DOCUMENTATION_BINARY_ROOTDIR}/${DOC_PUBLIC_API_FILENAME}.adoc
  WORKING_DIRECTORY ${DOCUMENTATION_BINARY_ROOTDIR}
  COMMENT "Converting public api adoc to html"
)

set (LIBDISIR_DOCUMENTATION_DIR share/doc/disir)
install (
  DIRECTORY ${DOCUMENTATION_BINARY_ROOTDIR}/
  DESTINATION ${LIBDISIR_DOCUMENTATION_DIR}
  COMPONENT Devel
)
