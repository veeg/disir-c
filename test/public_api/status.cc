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

TEST_F (StatusTest, fatal_context)
{
    status = DISIR_STATUS_FATAL_CONTEXT;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "FATAL CONTEXT");
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

TEST_F (StatusTest, wrong_value_type)
{
    status =  DISIR_STATUS_WRONG_VALUE_TYPE;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "WRONG VALUE TYPE");
}

TEST_F (StatusTest, not_exist)
{
    status =  DISIR_STATUS_NOT_EXIST;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "NOT EXIST");
}

TEST_F (StatusTest, restriction_violated)
{
    status = DISIR_STATUS_RESTRICTION_VIOLATED;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "RESTRICTION VIOLATED");
}

TEST_F (StatusTest, elements_invalid)
{
    status = DISIR_STATUS_ELEMENTS_INVALID;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "ELEMENTS INVALID");
}

TEST_F (StatusTest, not_supported)
{
    status = DISIR_STATUS_NOT_SUPPORTED;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "NOT SUPPORTED");
}

TEST_F (StatusTest, plugin_error)
{
    status = DISIR_STATUS_PLUGIN_ERROR;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "PLUGIN ERROR");
}

TEST_F (StatusTest, load_error)
{
    status = DISIR_STATUS_LOAD_ERROR;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "LOAD ERROR");
}

TEST_F (StatusTest, config_invalid)
{
    status = DISIR_STATUS_CONFIG_INVALID;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "CONFIG INVALID");
}

TEST_F (StatusTest, group_missing)
{
    status = DISIR_STATUS_GROUP_MISSING;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "GROUP MISSING");
}

TEST_F (StatusTest, permission_error)
{
    status = DISIR_STATUS_PERMISSION_ERROR;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "PERMISSION ERROR");
}

TEST_F (StatusTest, fs_error)
{
    status = DISIR_STATUS_FS_ERROR;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "FILESYSTEM ERROR");
}

TEST_F (StatusTest, default_missing)
{
    status = DISIR_STATUS_DEFAULT_MISSING;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "DEFAULT MISSING");
}

TEST_F (StatusTest, unknown)
{
    status = DISIR_STATUS_UNKNOWN;
    status_string = disir_status_string (status);
    ASSERT_STREQ (status_string, "UNKNOWN");
}

