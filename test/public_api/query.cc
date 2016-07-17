#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class QueryTest : public testing::DisirTestTestPlugin
{
    void SetUp()
    {
        DisirTestTestPlugin::SetUp ();

        status = DISIR_STATUS_OK;
        collection = NULL;
        mold = NULL;
        config = NULL;
        context = NULL;
        context_keyval = NULL;
        context_mold = NULL;
        context_section = NULL;
        context_config = NULL;

        status = disir_mold_input (instance, "test", "basic_keyval", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (mold != NULL);

        status = disir_mold_input (instance, "test", "basic_section", &section_mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (mold != NULL);

        context_mold = dc_mold_getcontext (mold);
        ASSERT_TRUE (context_mold != NULL);

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();

        if (mold)
        {
            status = disir_mold_finished (&mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_mold)
        {
            dc_putcontext (&context_mold);
        }

        if (context_keyval)
        {
            dc_destroy (&context_keyval);
        }
        if (config)
        {
            status = disir_config_finished (&config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (collection)
        {
            status = dc_collection_finished (&collection);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_config)
        {
            status = dc_destroy (&context_config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_section)
        {
            status = dc_destroy (&context_section);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        DisirTestTestPlugin::TearDown ();
    }

public:
    enum disir_status status;
    struct disir_context *context;
    struct disir_context *context_mold;
    struct disir_context *context_keyval;
    struct disir_context *context_section;
    struct disir_context *context_config;
    struct disir_mold   *mold;
    struct disir_mold   *section_mold;
    struct disir_config *config;
    struct disir_collection *collection;
    int32_t size;
    int i;
};

TEST_F (QueryTest, get_elements_invalid_argument)
{
    status = dc_get_elements (NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_elements (context_mold, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_elements (NULL, &collection);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (QueryTest, get_name_invalid_argument)
{
    const char *name;

    status = dc_get_name (NULL, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_name (context_keyval, NULL, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);

    status = dc_get_name (NULL, &name, NULL);
    EXPECT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

TEST_F (QueryTest, get_mold_keyval_contexts)
{
    status = dc_find_elements (context_mold, "key_integer", &collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (dc_collection_size (collection) == 1);

    status = dc_collection_next (collection, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (dc_context_type (context_keyval) == DISIR_CONTEXT_KEYVAL);
    ASSERT_TRUE (dc_value_type (context_keyval) == DISIR_VALUE_TYPE_INTEGER);

    // Cleanup collection_next context
    dc_putcontext (&context_keyval);
}


TEST_F (QueryTest, get_config_keyval_contexts)
{
    const char *out;
    int32_t size;

    status = dc_config_begin (mold, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config,
            DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "key_string", strlen ("key_string"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_string (context_keyval, "1", strlen ("1"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_find_elements (context_config, "key_string", &collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (dc_collection_size (collection) == 1);

    status = dc_collection_next (collection, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (dc_context_type (context_keyval) == DISIR_CONTEXT_KEYVAL);
    ASSERT_TRUE (dc_value_type (context_keyval) == DISIR_VALUE_TYPE_STRING);

    status = dc_get_value_string (context_keyval, &out, &size);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_STREQ ("1", out);

    // Put back keyval context
    dc_putcontext (&context_keyval);
}

TEST_F (QueryTest, get_section_and_keyval_contexts)
{
    const char *out;
    int32_t size;

    status = dc_config_begin (section_mold, &context_config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_section, "section_name", strlen ("section_name"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_section, DISIR_CONTEXT_KEYVAL, &context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "k1", strlen ("k1"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_string (context_keyval, "k1value", strlen ("k1value"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_keyval);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = dc_find_elements (context_section, "k1", &collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (dc_collection_size (collection) == 1);

    status = dc_collection_next (collection, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_TRUE (dc_context_type (context_keyval) == DISIR_CONTEXT_KEYVAL);
    ASSERT_TRUE (dc_value_type (context_keyval) == DISIR_VALUE_TYPE_STRING);

    status = dc_get_value_string (context_keyval, &out, &size);

    ASSERT_STREQ ("k1value", out);

    // cleanup collection_next
    dc_putcontext (&context_keyval);

    status = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status =  dc_collection_finished (&collection);
    ASSERT_STATUS  (DISIR_STATUS_OK, status);

    status = dc_find_elements (context_config, "section_name", &collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (dc_collection_size (collection) == 1);

    status = dc_collection_next (collection, &context_section);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_TRUE (dc_context_type (context_section) == DISIR_CONTEXT_SECTION);
}

