#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "log.h"

#define LOG_TEST_START \
    _log_disir_level(DISIR_LOG_LEVEL_TEST, "START TESTING %s()", __FUNCTION__);
#define LOG_TEST_CONTEXT_START(context) \
    _log_disir_level(DISIR_LOG_LEVEL_TEST, \
                     "START TESTING (context: %s) %s()", \
                     dc_type_string(context), __FUNCTION__)

#define LOG_TEST_END \
    _log_disir_level(DISIR_LOG_LEVEL_TEST, "END TESTING %s()", __FUNCTION__);
#define LOG_TEST_CONTEXT_END(context) \
    _log_disir_level(DISIR_LOG_LEVEL_TEST, \
                     "END TESTING (context: %s) %s()", \
                     dc_type_string(context), __FUNCTION__)

//! Include the individual test group source files.
#include "test_context.c"
#include "test_context_documentation.c"
#include "test_introduced.c"
#include "test_collection.c"
#include "test_util.c"
#include "test_element_storage.c"

int
main(int argc, char *argv[])
{
    int returnval = 0;

    //! Add one cmocka_run_group_tests_name per included test_*.c above.
    returnval += cmocka_run_group_tests_name("Context",
        disir_context_tests,
        NULL,
        NULL);

    returnval += cmocka_run_group_tests_name("Context-documentation",
        disir_context_documentation_tests,
        NULL,
        NULL);

    returnval += cmocka_run_group_tests_name("Introduced",
        disir_introduced_tests,
        NULL,
        NULL);

    returnval += cmocka_run_group_tests_name("Collection",
        disir_collection_tests,
        NULL,
        NULL);

    returnval += cmocka_run_group_tests_name("Utility",
        disir_util_tests,
        NULL,
        NULL);

    returnval += cmocka_run_group_tests_name("Element Storage",
        disir_element_storage_tests,
        NULL,
        NULL);

    return returnval;
}
