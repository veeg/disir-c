#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class StatusTest : public testing::Test
{
public:
    enum disir_status status;
    const char *status_string;

private:
    void SetUp()
    {
        status = DISIR_STATUS_OK;
    }
};


TEST_F (StatusTest, ok)
{
    status = DISIR_STATUS_OK;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "OK");
}

TEST_F (StatusTest, no_can_do)
{
    status = DISIR_STATUS_NO_CAN_DO;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "NO CAN DO");
}

TEST_F (StatusTest, too_few_arguments)
{
    status = DISIR_STATUS_TOO_FEW_ARGUMENTS;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "TOO FEW ARGUMENTS");
}

TEST_F (StatusTest, context_in_wrong_state)
{
    status = DISIR_STATUS_CONTEXT_IN_WRONG_STATE;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "CONTEXT IN WRONG STATE");
}

TEST_F (StatusTest, wrong_context)
{
    status = DISIR_STATUS_WRONG_CONTEXT;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "WRONG CONTEXT");
}

TEST_F (StatusTest, destroyed_context)
{
    status = DISIR_STATUS_DESTROYED_CONTEXT;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "DESTROYED CONTEXT");
}

TEST_F (StatusTest, bad_context_object)
{
    status = DISIR_STATUS_BAD_CONTEXT_OBJECT;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "BAD CONTEXT OBJECT");
}

TEST_F (StatusTest, invalid_context)
{
    status = DISIR_STATUS_INVALID_CONTEXT;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "INVALID CONTEXT");
}

TEST_F (StatusTest, no_memory)
{
    status = DISIR_STATUS_NO_MEMORY;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "NO MEMORY");
}

TEST_F (StatusTest, internal_error)
{
    status = DISIR_STATUS_INTERNAL_ERROR;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "INTERNAL ERROR");
}

TEST_F (StatusTest, insufficient_resources)
{
    status = DISIR_STATUS_INSUFFICIENT_RESOURCES;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "INSUFFICIENT RESOURCES");
}

TEST_F (StatusTest, exists)
{
    status = DISIR_STATUS_EXISTS;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "EXISTS");
}

TEST_F (StatusTest, conflicting_semver)
{
    status = DISIR_STATUS_CONFLICTING_SEMVER;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "CONFLICTING SEMVER");
}

TEST_F (StatusTest, conflict)
{
    status = DISIR_STATUS_CONFLICT;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "CONFLICT");
}

TEST_F (StatusTest, exhausted)
{
    status = DISIR_STATUS_EXHAUSTED;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "EXHAUSTED");
}

TEST_F (StatusTest, mold_missing)
{
    status =  DISIR_STATUS_MOLD_MISSING;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "MOLD MISSING");
}

TEST_F (StatusTest, unknown)
{
    status = DISIR_STATUS_UNKNOWN;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "UNKNOWN");
}

