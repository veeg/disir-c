#ifndef _LIBDISIR_TEST_HELPER_H
#define _LIBDISIR_TEST_HELPER_H

extern "C" {
#include "log.h"
}

#define LOG_TEST_START \
    _log_disir_level(DISIR_LOG_LEVEL_TEST, "START TESTING %s()", __FUNCTION__);
#define LOG_TEST_CONTEXT_START(context) \
    _log_disir_level(DISIR_LOG_LEVEL_TEST, \
                     "START TESTING (context: %s) %s()", \
                     dc_context_type_string (context), __FUNCTION__)

#define LOG_TEST_END \
    _log_disir_level(DISIR_LOG_LEVEL_TEST, "END TESTING %s()", __FUNCTION__);
#define LOG_TEST_CONTEXT_END(context) \
    _log_disir_level(DISIR_LOG_LEVEL_TEST, \
                     "END TESTING (context: %s) %s()", \
                     dc_context_type_string (context), __FUNCTION__)

#endif // _LIBDISIR_TEST_HELPER_H

