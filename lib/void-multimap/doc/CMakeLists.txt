

# Make 'doc' target
# configure doxugen config file
# Execute doxygen to build dir
# Execute converter on public header
# check in result, in doc folder

find_package (Doxygen)
# We alco require Doxml2AsciiDoc

if (DOXYGEN_FOUND)

    set (DOXYGEN_INPUT ${CMAKE_CURRENT_BINARY_DIR}/multimap.doxygen)
    set (DOXYGEN_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xml/multimap_8h.xml)
    set (DOXYGEN_INPUT_SOURCE_FILEPATH ${PROJECT_SOURCE_DIR}/src)
    set (ASCIIDOC_OUTPUT multimap_8h.adoc)
    set (HTML_OUTPUT multimap_8h.html)

    configure_file (
        ${CMAKE_CURRENT_SOURCE_DIR}/multimap.doxygen.in 
        ${CMAKE_CURRENT_BINARY_DIR}/${DOXYGEN_INPUT}
    )

    add_custom_command (
        OUTPUT ${DOXYGEN_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E echo_append "Building API Documentation..." 
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_INPUT}
        COMMAND ${CMAKE_COMMAND} -E echo "Done."
        DEPENDS ${DOXYGEN_INPUT}
    )

    add_custom_command (
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xml/${ASCIIDOC_OUTPUT}
        COMMAND ${PROJECT_SOURCE_DIR}/doxml2asciidoc/bin/doxml2asciidoc ${DOXYGEN_OUTPUT}
        DEPENDS ${DOXYGEN_OUTPUT}
    )

    add_custom_command (
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${ASCIIDOC_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/xml/${ASCIIDOC_OUTPUT} ${CMAKE_CURRENT_SOURCE_DIR}/${ASCIIDOC_OUTPUT}
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/xml/${ASCIIDOC_OUTPUT}
    )

    add_custom_command (
        OUTPUT ${HTML_OUTPUT}
        COMMAND asciidoctor ${ASCIIDOC_OUTPUT}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${ASCIIDOC_OUTPUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    add_custom_target (doc DEPENDS
        ${HTML_OUTPUT}
    )

endif (DOXYGEN_FOUND)
