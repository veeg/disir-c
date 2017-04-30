#include "test_json.h"
#include "json/input.h"
#include "json/output.h"

class InputOutputTest : public testing::JsonDioTestWrapper
{
    void TearDown()
    {
        DisirLogTestBodyExit();

        if (context_mold)
        {
            dc_putcontext (&context_mold);
        }
        if (config)
        {
            status = disir_config_finished (&config);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        if (context_config)
        {
            status = dc_destroy (&context_config);
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
        struct disir_config *config = NULL;
        struct disir_context *context_mold = NULL;
        enum disir_status status;
};

TEST_F (InputOutputTest, DISABLED_read_mold_shall_succeed)
{
    std::string path (m_jsonPath);
    path += "reference_mold.json";

    //status = dio_json_mold_read (instance, path.c_str (), &mold);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (InputOutputTest, DISABLED_write_mold_shall_succeed)
{
    std::string path (m_jsonPath);
    path += "reference_mold_write_test.json";

    status = disir_mold_read (instance, "test", "json_test_mold", &mold);
    ASSERT_TRUE (mold != NULL);

    //status = dio_json_mold_write (instance, path.c_str (), mold);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (InputOutputTest, DISABLED_read_config_shall_succeed)
{
    std::string path (m_jsonPath);
    path += "reference_config.json";

    status = disir_mold_read (instance, "test", "json_test_mold", &mold);
    ASSERT_TRUE (mold != NULL);

    //status = dio_json_config_read (instance, path.c_str (), mold, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (InputOutputTest, DISABLED_write_config_shall_succeed)
{
    std::string path (m_jsonPath);
    path += "reference_config_write_test.json";

    status = disir_mold_read (instance, "test", "json_test_mold", &mold);
    ASSERT_TRUE (mold != NULL);

    disir_generate_config_from_mold (mold, NULL, &config);

    //status = dio_json_config_write (instance, path.c_str (), config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (InputOutputTest, DISABLED_invalid_config_read)
{
    std::string path (m_jsonPath);
    path += "invalid_child_on_invalid_section.json";

    status = disir_mold_read (instance, "test", "json_test_mold", &mold);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    //status = dio_json_config_read (instance, path.c_str (), mold, &config);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, disir_config_valid (config, NULL))
}

