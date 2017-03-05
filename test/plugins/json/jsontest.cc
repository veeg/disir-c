#include <gtest/gtest.h>

#include <json/json.h>

#include "test_helper.h"

class JsonTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {

    }

    void TearDown()
    {

    }

};

TEST_F (JsonTest, DISABLED_indexing_json_key)
{
    Json::Value root;
    char *name;
    const char *val    = "HOLA";

    name = (char *)calloc(1, sizeof (char) * 20);
    ASSERT_TRUE (name != NULL);

    strcpy(name, "testkey");

    root[name] = val;
    std::cerr << root["testkey"].asString() << std::endl;
    ASSERT_STREQ (val, root["testkey"].asCString());
    free (name);
}


TEST_F (JsonTest, DISABLED_parsing_json)
{
    Json::Value root;
    Json::ValueIterator iter;

    bool ret = GetJsonConfig (root);
    ASSERT_TRUE (ret);

    ASSERT_TRUE (root["boolean"].type() == Json::booleanValue);

}
