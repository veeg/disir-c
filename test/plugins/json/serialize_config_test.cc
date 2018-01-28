#include "test_json.h"
#include "json/json_serialize.h"

class MarshallConfigTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        status = disir_mold_read (instance, "test", "json_test_mold", &mold);
        ASSERT_TRUE (mold != NULL);

        status = dc_config_begin (mold, &context_config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        writer = new dio::ConfigWriter (instance);

        DisirLogTestBodyEnter();
    }

    void TearDown()
    {
        DisirLogTestBodyExit();

        if (writer)
        {
            delete writer;
        }
        if (context_keyval)
        {
            status = dc_destroy (&context_keyval);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_section)
        {
            status = dc_destroy (&context_section);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_section_nested)
        {
            status = dc_destroy (&context_section_nested);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_config)
        {
            status = dc_destroy (&context_config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (config)
        {
            status = disir_config_finished (&config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (mold)
        {
            status = disir_mold_finished (&mold);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }

        DisirLogCurrentTestExit();
    }

    public:
        struct disir_mold *mold = NULL;
        struct disir_context *context_keyval = NULL;
        struct disir_context *context_config = NULL;
        struct disir_context *context_section = NULL;
        struct disir_context *context_section_nested = NULL;
        struct disir_config *config = NULL;
        dio::ConfigWriter *writer = NULL;
};

