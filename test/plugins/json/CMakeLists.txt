enable_testing()

find_package(GTest REQUIRED)

set (JSONCPP_ROOTDIR ${PROJECT_SOURCE_DIR}/lib/jsoncpp)

set (JSONIO_TEST_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}

    ${GTEST_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/json/include/
    ${JSONCPP_ROOTDIR}
)

set (TESTS_JSON_IO tests_json_io)
#file (GLOB TESTS_JSON_IO_SOURCES *.cc)

set(TESTS_JSON_IO_SOURCES
    "${JSONCPP_ROOTDIR}/jsoncpp.cpp"
    "jsontest.cc"
    "marshal_config.cc"
    "test_helper.cc"
    "unmarshal_config.cc"
    "marshal_mold.cc"
    "test_mold_input.cc"
    "unmarshal_mold_test.cc"
)


add_executable (${TESTS_JSON_IO} ${TESTS_JSON_IO_SOURCES})
set_property (TARGET ${TESTS_JSON_IO} PROPERTY CXX_STANDARD 11)
set_property (TARGET ${TESTS_JSON_IO} PROPERTY CXX_STANDARD_REQUIRED ON)

set_property (TARGET ${TESTS_JSON_IO} PROPERTY PREFIX "")

include_directories (
    ${JSONIO_TEST_INLCUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
    )

add_definitions ("-Wall")
add_definitions ("-Werror")
target_compile_definitions (${TESTS_JSON_IO} PRIVATE DEBUG=1)

configure_file ("test_helper.h.in" "test_helper.h")

include_directories (${JSONIO_TEST_INCLUDE_DIRS})

target_link_libraries (${TESTS_JSON_IO} dplugin_json)
target_link_libraries (${TESTS_JSON_IO} disir::disir)
target_link_libraries (${TESTS_JSON_IO} ${GTEST_BOTH_LIBRARIES})
target_link_libraries (${TESTS_JSON_IO} pthread)

add_test (JsonDIOTests ${TESTS_JSON_IO})
