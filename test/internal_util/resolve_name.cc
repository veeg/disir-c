#include <gtest/gtest.h>

// PRIVATE API
extern "C" {
#include "disir_private.h"
#include "context_private.h"
#include "query_private.h"
}

#include "test_helper.h"

class QueryResolveName : public testing::DisirTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();
        memset (name, 0, 2048);
        memset (resolved, 0, 2048);

        context = dx_context_create (DISIR_CONTEXT_CONFIG);
    }

    void TearDown()
    {
        DisirLogCurrentTestExit ();

        if (context)
        {
            dx_context_destroy (&context);
        }
    }

public:
    enum disir_status status;
    struct disir_context *context = NULL;
    char name[2048];
    char resolved[2048];

    char *next = NULL;
    int index = 0;
};

TEST_F (QueryResolveName, leading_key_seperator)
{
    sprintf (name, ".second");
    status = dx_query_resolve_name (context, name, resolved, &next, &index);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    ASSERT_STREQ ("'.' missing key before key seperator.", dc_context_error (context));
}

TEST_F (QueryResolveName, leading_index_indicator)
{
    sprintf (name, "@4.second");
    status = dx_query_resolve_name (context, name, resolved, &next, &index);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    ASSERT_STREQ ("'@' missing key before index indicator.", dc_context_error (context));
}

TEST_F (QueryResolveName, missing_index_after_indicator)
{
    sprintf (name, "first@.second");
    status = dx_query_resolve_name (context, name, resolved, &next, &index);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    ASSERT_STREQ ("'first@' missing index after indicator.", dc_context_error (context));
}

TEST_F (QueryResolveName, invalid_index_after_indicator)
{
    sprintf (name, "first@abc.second");
    status = dx_query_resolve_name (context, name, resolved, &next, &index);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    ASSERT_STREQ ("'first@abc' is an invalid index indicator.", dc_context_error (context));
}

TEST_F (QueryResolveName, invalid_trailing_character_after_index)
{
    sprintf (name, "first@14abc.second");
    status = dx_query_resolve_name (context, name, resolved, &next, &index);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    ASSERT_STREQ ("'first@14abc' contains additional trailing non-integer characters.",
                  dc_context_error (context));
}

TEST_F (QueryResolveName, single_valid_key_no_index)
{
    sprintf (name, "first");
    status = dx_query_resolve_name (context, name, resolved, &next, &index);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("first", name);
    ASSERT_EQ (0, index);
    ASSERT_TRUE (next == NULL);

    ASSERT_TRUE (dc_context_error (context) == NULL);
}

TEST_F (QueryResolveName, single_valid_key_with_index)
{
    sprintf (name, "first@5");
    status = dx_query_resolve_name (context, name, resolved, &next, &index);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("first", name);
    ASSERT_EQ (5, index);
    ASSERT_TRUE (next == NULL);

    ASSERT_TRUE (dc_context_error (context) == NULL);
}

TEST_F (QueryResolveName, next_key_resolved_no_index)
{
    sprintf (name, "first.second");
    status = dx_query_resolve_name (context, name, resolved, &next, &index);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("first", name);
    ASSERT_TRUE (next != NULL);
    ASSERT_STREQ ("second", next);
}

TEST_F (QueryResolveName, next_key_resolved_with_index)
{
    sprintf (name, "first@3.second");
    status = dx_query_resolve_name (context, name, resolved, &next, &index);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("first", name);
    ASSERT_TRUE (next != NULL);
    ASSERT_STREQ ("second", next);
}

