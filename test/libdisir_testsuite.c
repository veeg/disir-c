#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "log.h"

#define LOG_TEST _log_disir_level(DISIR_LOG_LEVEL_TEST, "TESTING %s()", __FUNCTION__);
#define LOG_TEST_CONTEXT(context) _log_disir_level(DISIR_LOG_LEVEL_TEST, "TESTING (context: %s) %s()", dc_type_string(context), __FUNCTION__)

//! Include the individual test group source files.
#include "test_context.c"
#include "test_context_documentation.c"
#include "test_introduced.c"

int
main(int argc, char *argv[])
{
    int returnval = 0;

    //! Add one cmocka_run_group_tests_name per included test_*.c above.
    returnval += cmocka_run_group_tests_name("Context", disir_context_tests, NULL, NULL);
    returnval += cmocka_run_group_tests_name("Context-documentation", disir_context_documentation_tests, NULL, NULL);
    returnval += cmocka_run_group_tests_name("Introduced", disir_introduced_tests, NULL, NULL);
    return returnval;
}
