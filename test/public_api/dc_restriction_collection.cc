// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


//
// This class tests the public API functions:
//  dc_restriction_collection
//
class RestrictionCollection : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp();

        DisirLogTestBodyEnter();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (collection)
        {
            dc_collection_finished (&collection);
        }
        if (context)
        {
            dc_putcontext (&context);
        }
        if (context_config)
        {
            dc_putcontext (&context_config);
        }
        if (context_keyval)
        {
            dc_putcontext (&context_keyval);
        }
        if (context_mold)
        {
            dc_putcontext (&context_mold);
        }
        if (config)
        {
            disir_config_finished (&config);
        }
        if (mold)
        {
            disir_mold_finished (&mold);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status = DISIR_STATUS_OK;
    enum disir_restriction_type restriction_type = DISIR_RESTRICTION_UNKNOWN;
    struct disir_config *config = NULL;
    struct disir_mold *mold = NULL;
    struct disir_context *context = NULL;
    struct disir_context *context_config = NULL;
    struct disir_context *context_keyval = NULL;
    struct disir_context *context_mold = NULL;
    struct disir_collection *collection = NULL;
};

TEST_F (RestrictionCollection, invalid_arguments)
{
    ASSERT_NO_SETUP_FAILURE();

    status = dc_restriction_collection (NULL, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);


    status = disir_mold_read (instance, "test",
                              "restriction_config_parent_keyval_max_entry", &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    context_mold = dc_mold_getcontext (mold);

    status = dc_restriction_collection (context_mold, NULL);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_restriction_collection (NULL, &collection);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (RestrictionCollection, context_mold_wrong_context)
{
    ASSERT_NO_SETUP_FAILURE();

    status = disir_mold_read (instance, "test",
                              "restriction_config_parent_keyval_max_entry", &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    context_mold = dc_mold_getcontext (mold);

    status = dc_restriction_collection (context_mold, &collection);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (RestrictionCollection, context_config_wrong_context)
{
    ASSERT_NO_SETUP_FAILURE();

    status = disir_config_read (instance, "test",
                                "restriction_config_parent_keyval_max_entry", NULL, &config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    context_config = dc_config_getcontext (config);

    status = dc_restriction_collection (context_config, &collection);
    ASSERT_STATUS (DISIR_STATUS_WRONG_CONTEXT, status);
}

TEST_F (RestrictionCollection, mold_context_keyval_no_restrictions)
{
    ASSERT_NO_SETUP_FAILURE();

    status = disir_mold_read (instance, "test", "basic_keyval", &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    context_mold = dc_mold_getcontext (mold);

    status = dc_find_element (context_mold, "key_string", 0, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // We expect the call to be OK, but the collection is empty (NULL)
    status = dc_restriction_collection (context_keyval, &collection);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
    ASSERT_TRUE (collection == NULL);
}

TEST_F (RestrictionCollection, config_context_keyval_two_restrictions_entries_max)
{
    ASSERT_NO_SETUP_FAILURE();

    status = disir_config_read (instance, "test",
                                "restriction_config_parent_keyval_max_entry", NULL, &config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    context_config = dc_config_getcontext (config);

    status = dc_find_element (context_config, "keyval", 0, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_restriction_collection (context_keyval, &collection);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (collection != NULL);
    ASSERT_EQ (2, dc_collection_size (collection));

    // Further asserts on the content is handled by
    //   mold_context_keyval_two_restriction_entries_max
}

TEST_F (RestrictionCollection, mold_context_keyval_two_restrictions_entries_max)
{
    ASSERT_NO_SETUP_FAILURE();

    status = disir_mold_read (instance, "test",
                              "restriction_config_parent_keyval_max_entry", &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    context_mold = dc_mold_getcontext (mold);

    status = dc_find_element (context_mold, "keyval", 0, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_restriction_collection (context_keyval, &collection);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (collection != NULL);
    ASSERT_EQ (2, dc_collection_size (collection));

    status = dc_collection_next (collection, &context);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (context != NULL);

    status = dc_get_restriction_type (context, &restriction_type);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (DISIR_RESTRICTION_INC_ENTRY_MAX, restriction_type);
    dc_putcontext (&context);

    status = dc_collection_next (collection, &context);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (context != NULL);

    status = dc_get_restriction_type (context, &restriction_type);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_EQ (DISIR_RESTRICTION_INC_ENTRY_MAX, restriction_type);
    dc_putcontext (&context);

    status = dc_collection_next (collection, &context);
    ASSERT_STATUS (DISIR_STATUS_EXHAUSTED, status);
}

