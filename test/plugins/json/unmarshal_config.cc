#include <gtest/gtest.h>
#include <disir/disir.h>
#include <json/json.h>
#include "test_helper.h"
#include "log.h"
#include "input.h"
#include "output.h"
#include "dplugin_json.h"

class UnMarshallConfigTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
        SetUpDisir ();

        mold = NULL;
        config = NULL;
        reader = NULL;
        writer = NULL;
        collection = NULL;
        context_config = NULL;

        status = disir_mold_input (disir, "test", "json_test_mold", &mold);
        ASSERT_TRUE (mold != NULL);

        reader = new dio::ConfigReader (disir, mold);
        writer = new dio::ConfigWriter (disir);
    }
    void TearDown()
    {
        if (reader)
        {
            delete reader;
        }
        if (writer)
        {
            delete writer;
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
        if (collection)
        {
            dc_collection_finished (&collection);
            EXPECT_STATUS (DISIR_STATUS_OK, status);
        }
        TearDownDisir ();
    }

    public:
        struct disir_mold *mold;
        struct disir_context *context_config;
        struct disir_config *config;
        struct disir_collection *collection;
        enum dplugin_status pstatus;
        enum disir_status status;
        dio::ConfigWriter *writer;
        dio::ConfigReader *reader;
};

TEST_F(UnMarshallConfigTest, unmarshal_succeed)
{
    Json::Value root;
    Json::StyledWriter Jwriter;
    std::string result, expected;
    std::string config_json_marshalled;
    bool success = GetJsonConfig (root);
    ASSERT_TRUE (success);
    result = Jwriter.writeOrdered (root);

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, m_configjsonPath.c_str ());
    });

    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);
    ASSERT_TRUE (writer->marshal (config, expected) == DPLUGIN_STATUS_OK);
    ASSERT_STREQ (result.c_str(), expected.c_str());
}

TEST_F(UnMarshallConfigTest, version_should_be_correct)
{
    struct semantic_version expected;
    struct semantic_version actual;

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, std::string ("{\"version\" : \"1.0.0\"}"));
    });
    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);

    status = dc_semantic_version_convert ("1.0.0", &expected);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    context_config = dc_config_getcontext (config);

    status = dc_get_version (context_config, &actual);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    dc_putcontext (&context_config);

    ASSERT_EQ (0, dc_semantic_version_compare (&expected, &actual));
}

TEST_F(UnMarshallConfigTest, no_version_should_result_standard)
{
    struct semantic_version expected;
    struct semantic_version actual;

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, std::string ("{\"noversion\" : \"1.0.0\"}"));
    });

    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);

    status = dc_semantic_version_convert ("1.0.0", &expected);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    context_config = dc_config_getcontext (config);

    status = dc_get_version (context_config, &actual);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    dc_putcontext (&context_config);

    ASSERT_EQ (0, dc_semantic_version_compare (&expected, &actual));
}

TEST_F(UnMarshallConfigTest, unmarshal_returns_parse_error)
{
    std::string invalid ("invalid_json");

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, invalid);
    });
    ASSERT_EQ (DPLUGIN_PARSE_ERROR, pstatus);
}

TEST_F(UnMarshallConfigTest, return_error_on_invalid_path)
{
    std::string output_s;

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, "fake");
    });
    ASSERT_EQ (DPLUGIN_IO_ERROR, pstatus);
}

TEST_F (UnMarshallConfigTest, invalid_context_on_wrong_value)
{
    std::string filepath (m_jsonPath);

    filepath += "wrong_value_in_config.json";

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, filepath.c_str ());
    });
    std::cout << disir_error (disir) << std::endl;
    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);

    ASSERT_EQ (true, check_context_validity (config, "boolean"));
}

TEST_F (UnMarshallConfigTest, invalid_context_on_wrong_key)
{
    std::string filepath (m_jsonPath);

    filepath += "wrong_name.json";

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, filepath.c_str ());
    });
    std::cerr << disir_error (disir) << std::endl;
    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);

    ASSERT_EQ (true, check_context_validity (config, "noname"));
}

TEST_F (UnMarshallConfigTest, DISABLED_invalid_context_section_on_wrong_key)
{
    std::string filepath (m_jsonPath);

    filepath += "fake_section.json";

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, filepath.c_str());
    });
    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);

    ASSERT_EQ (true, check_context_validity (config, "noname"));
}

TEST_F (UnMarshallConfigTest, parse_duplicate_keyvals)
{
    std::string filepath (m_jsonPath);

    filepath += "duplicate_keyvals.json";

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, filepath.c_str());
    });

    context_config = dc_config_getcontext (config);

    status = dc_get_elements (context_config, &collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (2, dc_collection_size (collection));

    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);
}

TEST_F (UnMarshallConfigTest, parse_duplicate_sections)
{
    std::string filepath (m_jsonPath);

    filepath += "duplicate_sections.json";

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, filepath.c_str());
    });
    std::cerr << disir_error (disir) << std::endl;
    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);

    context_config = dc_config_getcontext (config);

    status = dc_get_elements (context_config, &collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_EQ (2, dc_collection_size (collection));

    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);
}


TEST_F (UnMarshallConfigTest, invalid_section_shall_succeed)
{
    std::string filepath (m_jsonPath);

    filepath += "invalid_section.json";

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, filepath.c_str());
    });
    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);

    ASSERT_EQ (true, check_context_validity (config, "fake_section"));
}

TEST_F (UnMarshallConfigTest, float_value_conversion)
{
    struct disir_collection *collection;
    struct disir_context *context_keyval_float;
    double value;
    std::string path (m_jsonPath);
    path += "reference_config.json";

     reader->unmarshal (&config, path.c_str ());

     context_config = dc_config_getcontext (config);
     ASSERT_TRUE (context_config != NULL);

     status = dc_find_elements (context_config, "float", &collection);
     EXPECT_STATUS (DISIR_STATUS_OK, status);

     status = dc_collection_next (collection, &context_keyval_float);
     EXPECT_STATUS (DISIR_STATUS_OK, status);

     status = dc_get_value_float (context_keyval_float, &value);
     EXPECT_STATUS (DISIR_STATUS_OK, status);

     printf ("%f\n", value);

     ASSERT_EQ (12.123, value);
}


TEST_F (UnMarshallConfigTest, invalid_keyval_on_invalid_parent)
{
    std::string filepath (m_jsonPath);
    struct disir_collection *collection;
    struct disir_context *invalid_section;

    filepath += "invalid_child_on_invalid_section.json";

    EXPECT_NO_THROW ({
        pstatus = reader->unmarshal (&config, filepath.c_str());
    });
    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);

    ASSERT_EQ (true, check_context_validity (config, "fake_section"));

    context_config = dc_config_getcontext (config);

    status = dc_get_elements (context_config, &collection);
    ASSERT_EQ (DISIR_STATUS_OK, status);

    status = dc_collection_next (collection, &invalid_section);
    ASSERT_EQ (DISIR_STATUS_OK, status);

    ASSERT_EQ (true, check_context_validity (invalid_section, "unimportant_name"));

    dc_collection_finished (&collection);
}
