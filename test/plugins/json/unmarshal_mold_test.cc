#include <gtest/gtest.h>
#include <disir/disir.h>
#include <json/json.h>
#include "test_helper.h"
#include "log.h"
#include "input.h"
#include "output.h"
#include "dplugin_json.h"

class UnMarshallMoldTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
        SetUpDisir ();

        mold = NULL;
        config = NULL;
        context_mold = NULL;
        context_config = NULL;

        reader = new dio::MoldReader (disir);
        writer = new dio::MoldWriter (disir);

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
        dio::MoldReader *reader;
        dio::MoldWriter *writer;
};

TEST_F (UnMarshallMoldTest, read_mold_shall_succeed)
{
    enum dplugin_status status;
    status = reader->unmarshal (m_moldjsonPath.c_str(), &mold);
    ASSERT_EQ (DPLUGIN_STATUS_OK, status);
}

TEST_F (UnMarshallMoldTest, assert_output)
{
    std::string actual;
    enum dplugin_status status;
    status = reader->unmarshal (m_moldjsonPath.c_str(), &mold);
    ASSERT_EQ (DPLUGIN_STATUS_OK, status);

    status = writer->marshal (mold, actual);
    ASSERT_EQ (DPLUGIN_STATUS_OK, status);
    ASSERT_TRUE (compare_json_objects (getMoldJson(), actual));

}

TEST_F (UnMarshallMoldTest, crash_on_absent_mold_version)
{
    enum dplugin_status status;
    std::string path (m_jsonPath);
    path += "no_version_mold.json";

    status = reader->unmarshal (path.c_str (), &mold);
    ASSERT_EQ (DPLUGIN_FATAL_ERROR, status);
}

TEST_F (UnMarshallMoldTest, crash_on_absent_defaults)
{
    enum dplugin_status status;
    std::string path (m_jsonPath);
    path += "unpresent_mold_defaults.json";

    status = reader->unmarshal (path.c_str (), &mold);
    ASSERT_EQ (DPLUGIN_FATAL_ERROR, status);
    std::cerr << disir_error (disir) << std::endl;
}

TEST_F (UnMarshallMoldTest, crash_on_absent_type)
{
    enum dplugin_status status;
    std::string path (m_jsonPath);
    path += "no_type_mold.json";

    status = reader->unmarshal (path.c_str (), &mold);
    ASSERT_EQ (DPLUGIN_FATAL_ERROR, status);
    std::cerr << disir_error (disir) << std::endl;
}

TEST_F (UnMarshallMoldTest, crash_on_absent_introduced)
{
    enum dplugin_status status;
    std::string path (m_jsonPath);
    path += "no_version_mold.json";

    status = reader->unmarshal (path.c_str (), &mold);
    ASSERT_EQ (DPLUGIN_FATAL_ERROR, status);
    std::cerr << disir_error (disir) << std::endl;
}

TEST_F (UnMarshallMoldTest, crash_on_absent_elements)
{
    enum dplugin_status status;
    std::string path (m_jsonPath);
    path += "absent_elements.json";

    status = reader->unmarshal (path.c_str (), &mold);
    ASSERT_EQ (DPLUGIN_FATAL_ERROR, status);
    std::cerr << disir_error (disir) << std::endl;

}

TEST_F (UnMarshallMoldTest, integer_value_conversion)
{


}

TEST_F (UnMarshallMoldTest, correct_documentation)
{
    enum dplugin_status pstatus;
    enum disir_status status;
    std::string path (m_jsonPath);
    const char *doc;

    path += "mold_documentation.json";

    pstatus = reader->unmarshal (path.c_str (), &mold);
    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);

    context_mold = dc_mold_getcontext (mold);
    ASSERT_TRUE (context_mold != NULL);

    status = dc_get_documentation (context_mold, NULL, &doc, NULL);
    std::cerr << disir_error (disir) << std::endl;
    ASSERT_EQ (DISIR_STATUS_OK, status);

    ASSERT_STREQ ("this is a mold documentation", doc);
}

