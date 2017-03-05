#include <gtest/gtest.h>
#include <disir/disir.h>
#include <json/json.h>
#include "test_helper.h"
#include "log.h"
#include "input.h"
#include "output.h"
#include "dplugin_json.h"

class InputOutputTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
        SetUpDisir ();

        mold = NULL;
        config = NULL;
        context_mold = NULL;
        context_config = NULL;

    }
    void TearDown()
    {
        delete reader;
        delete writer;

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
        TearDownDisir ();
    }

    public:
        struct disir_mold *mold;
        struct disir_context *context_config;
        struct disir_config *config;
        struct disir_context *context_mold;
        enum dplugin_status pstatus;
        enum disir_status status;
        dio::MoldReader *reader;
        dio::MoldWriter *writer;
};

TEST_F (InputOutputTest, read_mold_shall_succeed)
{
    std::string path (m_jsonPath);
    path += "reference_mold.json";

    auto status = dio_json_mold_read (disir, path.c_str (), &mold);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (InputOutputTest, write_mold_shall_succeed)
{
    std::string path (m_jsonPath);
    path += "reference_mold_write_test.json";

    auto status = disir_mold_input (disir, "test", "json_test_mold", &mold);
    ASSERT_TRUE (mold != NULL);

    status = dio_json_mold_write (disir, path.c_str (), mold);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (InputOutputTest, read_config_shall_succeed)
{
    std::string path (m_jsonPath);
    path += "reference_config.json";

    disir_mold_input (disir, "test", "json_test_mold", &mold);

    auto status = dio_json_config_read (disir, path.c_str (), mold, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (InputOutputTest, write_config_shall_succeed)
{
    std::string path (m_jsonPath);
    path += "reference_config_write_test.json";

    disir_mold_input (disir, "test", "json_test_mold", &mold);

    disir_generate_config_from_mold (mold, NULL, &config);

    auto status = dio_json_config_write (disir, path.c_str (), config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (InputOutputTest, invalid_config_read)
{
    std::string path (m_jsonPath);
    path += "invalid_child_on_invalid_section.json";

    status = disir_mold_input (disir, "test", "json_test_mold", &mold);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dio_json_config_read (disir, path.c_str (), mold, &config);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, disir_config_valid (config, NULL))
}

