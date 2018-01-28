#include "test_json.h"
#include "json/json_serialize.h"
#include "json/json_unserialize.h"

class UnserializeConfigTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        status = disir_mold_read (instance, "test", "json_test_mold", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (mold != NULL);

        status = disir_config_read (instance, "test", "json_test_mold", NULL, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
        ASSERT_TRUE (config != NULL);

        writer = new dio::ConfigWriter (instance);
        reader = new dio::ConfigReader (instance, mold);

        std::stringstream serialized_config;

        status = writer->serialize (config, serialized_config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        bool ret = json_reader.parse (serialized_config.str(), root);
        ASSERT_EQ (true, ret) << json_reader.getFormattedErrorMessages();

        disir_config_finished (&config);

        DisirLogTestBodyEnter();
    }

    void TearDown()
    {
        DisirLogTestBodyExit();

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
        DisirLogCurrentTestExit();
    }
public:
    enum disir_status unserialize_config ()
    {
        Json::StyledWriter json_writer;

        return reader->unserialize (&config, json_writer.writeOrdered (root));
    }

public:
    struct disir_mold *mold = NULL;
    struct disir_config *config = NULL;
    enum disir_status status;
    dio::ConfigWriter *writer = NULL;
    dio::ConfigReader *reader = NULL;
    Json::Value root;
    Json::Reader json_reader;

};

TEST_F(UnserializeConfigTest, unserialize_shall_succeed)
{
    status = unserialize_config();
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (UnserializeConfigTest, element_not_in_mold)
{
    ASSERT_NO_THROW (
        root["config"]["invalid"] = "invalid";
    );

    status = unserialize_config();
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (UnserializeConfigTest, keyval_invalid_type_int)
{
    ASSERT_NO_THROW (
        root["config"]["test2"] = 2;
    );

    status = unserialize_config();
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (UnserializeConfigTest, wrong_value_type_in_keyval_array)
{
    ASSERT_NO_THROW (
        root["config"]["test1"] = Json::arrayValue;
        root["config"]["test1"][0] = 1;
    );

    status = unserialize_config();
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (UnserializeConfigTest, invalid_type_version)
{
    ASSERT_NO_THROW (
        root["version"] = 2;
    );

    status = unserialize_config();
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (UnserializeConfigTest, invalid_type)
{
    ASSERT_NO_THROW (
        root["config"]["test1"].append (2);
    );

    status = unserialize_config ();
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);
}

TEST_F (UnserializeConfigTest, float_as_int_shall_succeed)
{
    ASSERT_NO_THROW (
        root["config"]["float"] = 15;
    );

    status = unserialize_config();
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (UnserializeConfigTest, empty_section)
{
    ASSERT_NO_THROW (
        root["config"]["empty_section"] = Json::objectValue;
    );

    status = unserialize_config();
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}
