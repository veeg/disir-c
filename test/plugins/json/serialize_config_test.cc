#include "test_json.h"
#include "json/json_serialize.h"

class MarshallConfigTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        status = disir_mold_read (instance, "test", "json_test_mold", &mold);
        ASSERT_TRUE (mold != NULL);

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
        if (context_section)
        {
            status = dc_destroy (&context_section);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_config)
        {
            status = dc_putcontext (&context_config);
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
        struct disir_context *context_config = NULL;
        struct disir_context *context_section = NULL;
        struct disir_config *config = NULL;
        dio::ConfigWriter *writer = NULL;
};

TEST_F(MarshallConfigTest, empty_section)
{
    status = disir_generate_config_from_mold (mold, NULL, &config);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    context_config = dc_config_getcontext (config);
    // find empty_section, delete it
    status = dc_query_resolve_context(context_config, "empty_section", &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_destroy (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Lets construct an empty one
    status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section, "empty_section", strlen("empty_section"));
    ASSERT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_section);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    // Serialize this config!
    std::string serialized;
    status = writer->serialize (config, serialized);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    Json::Value root;
    Json::Reader reader;
    bool success = reader.parse (serialized, root);
    ASSERT_TRUE (success);

    ASSERT_NO_THROW (
        Json::Value empty = Json::objectValue;
        ASSERT_FALSE(root["config"]["empty_section"].isNull());
        ASSERT_EQ(empty, root["config"]["empty_section"])
            << "expected objectValue in serialized empty_section, found: "
            << root["empty_section"];
    );
}
