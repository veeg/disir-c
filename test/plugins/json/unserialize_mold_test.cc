#include "test_json.h"
#include "json/json_serialize.h"
#include "json/json_unserialize.h"

class UnMarshallMoldTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        reader = new dio::MoldReader (instance);
        writer = new dio::MoldWriter (instance);

        DisirLogTestBodyEnter();
    }

    void TearDown()
    {
        DisirLogTestBodyExit();

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

        DisirLogCurrentTestExit();
    }

    public:
        struct disir_mold *mold = NULL;
        struct disir_context *context_config = NULL;
        struct disir_config *config = NULL;
        struct disir_context *context_mold = NULL;
        dio::MoldReader *reader = NULL;
        dio::MoldWriter *writer = NULL;
};

TEST_F (UnMarshallMoldTest, read_mold_shall_succeed)
{
    status = reader->unserialize (m_moldjsonPath.c_str(), &mold);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (UnMarshallMoldTest, crash_on_absent_mold_version)
{
    std::string path (m_jsonPath);
    path += "no_version_mold.json";

    status = reader->unserialize (path.c_str (), &mold);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (UnMarshallMoldTest, crash_on_absent_defaults)
{
    std::string path (m_jsonPath);
    path += "unpresent_mold_defaults.json";

    status = reader->unserialize (path.c_str (), &mold);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (UnMarshallMoldTest, crash_on_absent_type)
{
    std::string path (m_jsonPath);
    path += "no_type_mold.json";

    status = reader->unserialize (path.c_str (), &mold);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (UnMarshallMoldTest, crash_on_absent_introduced)
{
    std::string path (m_jsonPath);
    path += "no_version_mold.json";

    status = reader->unserialize (path.c_str (), &mold);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (UnMarshallMoldTest, crash_on_absent_elements)
{
    std::string path (m_jsonPath);
    path += "absent_elements.json";

    status = reader->unserialize (path.c_str (), &mold);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

