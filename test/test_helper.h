#ifndef _LIBDISIR_TEST_HELPER_H
#define _LIBDISIR_TEST_HELPER_H

extern "C" {
#include "log.h"
}

#define ASSERT_STATUS(a,b)                                                  \
    {                                                                       \
        if (a != b)                                                         \
        {                                                                   \
            _log_disir_level (DISIR_LOG_LEVEL_TEST,                         \
                              "STATUS ASSERT FAILED %s vs %s (%s:%d)",      \
                              disir_status_string (a),                      \
                              disir_status_string (b),                      \
                              __FUNCTION__, __LINE__                        \
                             );                                             \
            FAIL() << "Expected status '"                                   \
                   << disir_status_string (a)                               \
                   << " (" << a << ")'"                                     \
                   << ", got '"                                             \
                   << disir_status_string (b)                               \
                   << " (" << b << ")'";                                    \
        }                                                                   \
    }

#define EXPECT_STATUS(a,b)                                                  \
    {                                                                       \
        if (a != b)                                                         \
        {                                                                   \
            _log_disir_level (DISIR_LOG_LEVEL_TEST,                         \
                              "STATUS ASSERT FAILED %s vs %s (%s:%d)",      \
                              disir_status_string (a),                      \
                              disir_status_string (b),                      \
                              __FUNCTION__, __LINE__                        \
                             );                                             \
            ADD_FAILURE() << "Expected status '"                            \
                   << disir_status_string (a)                               \
                   << " (" << a << ")'"                                     \
                   << ", got '"                                             \
                   << disir_status_string (b)                               \
                   << " (" << b << ")'";                                    \
        }                                                                   \
    }


namespace testing
{
    class DisirTestWrapper : public testing::Test
    {
    protected:
        void DisirLogCurrentTestEnter()
        {
            // Log current test running
            const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
            if (test_info != NULL)
            {
                _log_disir_level (DISIR_LOG_LEVEL_TEST, "STARTING TEST %s.%s",
                                  test_info->test_case_name(), test_info->name());
            }
        }

        void DisirLogCurrentTestExit()
        {
            // Log current test running
            const ::testing::TestInfo* const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();
            if (test_info != NULL)
            {
                _log_disir_level (DISIR_LOG_LEVEL_TEST, "EXITING TEST %s.%s",
                                  test_info->test_case_name(), test_info->name());
            }
        }
    };
}

#endif // _LIBDISIR_TEST_HELPER_H

