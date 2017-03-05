#include <gtest/gtest.h>
#include <disir/disir.h>
#include <json/json.h>
#include "test_helper.h"
#include "log.h"
#include "output.h"

class MarshallConfigTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
       mold = NULL;
       config = NULL;
       context_keyval = NULL;
       context_config = NULL;
       context_section = NULL;
       context_section_nested = NULL;

       SetUpDisir ();

       status = disir_mold_input (disir, "test", "json_test_mold", &mold);
       ASSERT_TRUE (mold != NULL);

       status = dc_config_begin (mold, &context_config);
       EXPECT_STATUS (DISIR_STATUS_OK, status);

       writer = new dio::ConfigWriter (disir);
    }
    void TearDown()
    {
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

        TearDownDisir ();
    }

    public:
        struct disir_mold *mold;
        struct disir_context *context_keyval;
        struct disir_context *context_config;
        struct disir_context *context_section;
        struct disir_context *context_section_nested;
        struct disir_config *config;
        dio::ConfigWriter *writer;
};

TEST_F(MarshallConfigTest, no_keyval_context)
{
    std::string json, expected, result;
    bool success;

    success = formatJson (expected,"{\n\"version\" : \"1.0.0\", \
                                     \"config\" : {\"test1\":\"k1val\"}}");
    ASSERT_TRUE (success);

    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "test1", strlen ("test1"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_value_string (context_keyval, "k1val", strlen ("k1val"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_config_finalize (&context_config, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    auto pstatus = writer->marshal (config, json);
    ASSERT_TRUE (pstatus == DPLUGIN_STATUS_OK);

    // Checking if we got a valid json string
    success = formatJson (result, json);
    std::cerr << disir_error (disir) << std::endl;
    ASSERT_TRUE (success);

    ASSERT_STREQ (expected.c_str(), result.c_str());
}

/* Config layout in json
         * {
         *      "section_name" : {
         *        "k1" : "k1value",
         *        "k2" : "k2value"
         *        "section2" : {
         *          "k3" : "k3value"
         *        }
         *      }
         *}
         */

TEST_F(MarshallConfigTest, marshall_section_shall_pass)
{
    std::string json, expected;
    bool success;

    success = formatJson (expected,
            "{\"version\" : \"1.0.0\",  \
            \"config\" : {              \
            \"section_name\": {         \
            \"k1\" : \"k1value\",       \
            \"k2\" : \"k2value\",       \
            \"section2\" : {            \
             \"k3\" : \"k3value\"       \
             } \
            }}}");
    ASSERT_TRUE (success);


    status = dc_begin (context_config,
            DISIR_CONTEXT_SECTION, &context_section);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section, "section_name", strlen ("section_name"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_begin (context_section, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "k1", strlen ("k1"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_value_string (context_keyval, "k1value", strlen ("k1value"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_begin (context_section, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "k2", strlen ("k2"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_value_string (context_keyval, "k2value", strlen ("k2value"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_begin (context_section,
            DISIR_CONTEXT_SECTION, &context_section_nested);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_begin (context_section_nested, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS  (DISIR_STATUS_OK, status);
    status = dc_set_name (context_section_nested, "section2", strlen ("section2"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_name (context_keyval, "k3", strlen ("k3"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_set_value_string (context_keyval, "k3value", strlen ("k3value"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_section_nested);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_finalize (&context_section);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_config_finalize (&context_config, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    auto pstatus = writer->marshal (config, json);
    ASSERT_TRUE (pstatus == DPLUGIN_STATUS_OK);
    ASSERT_STREQ (expected.c_str(), json.c_str());
}


TEST_F(MarshallConfigTest, empty_config_shall_fail)
{
    std::string empty;

    status = dc_config_finalize (&context_config, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    ASSERT_FALSE (writer->marshal (config, empty) != DPLUGIN_STATUS_OK);
}
/*
{
    "string" : "value",
    "integer" : 123,
    "float"   : 12.123,
    "boolean" : true
}
*/
TEST_F(MarshallConfigTest, marshall_all_types)
{
   std::string json;
   std::string expected;
   bool success;
   int64_t integer = 123;

    success = formatJson (expected,
            "{\"version\" : \"1.0.0\",  \
            \"config\" : {              \
            \"test\": \"1\",            \
            \"integer\" : 123,          \
            \"float\" : 12.123,         \
            \"boolean\" : true          \
            }}");
    ASSERT_TRUE (success);

    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "test1", strlen ("test"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_string (context_keyval, "1", strlen ("1"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "integer", strlen ("integer"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_integer (context_keyval, integer);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "float", strlen ("float"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_float (context_keyval, (double)12.123);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config,
            DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "boolean", strlen ("boolean"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_boolean (context_keyval, (uint8_t)1);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_config_finalize (&context_config, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_TRUE (writer->marshal(config, json) == DPLUGIN_STATUS_OK);
    ASSERT_STREQ (expected.c_str(), json.c_str());
}

TEST_F (MarshallConfigTest, duplicate_keyvals_shall_succeed)
{
    std::string json;
    std::string expected;

    formatJson (expected,
            "{\"version\" : \"1.0.0\", \
                \"config\" : {          \
                \"test1\" :  \"value1\", \
                \"test1@2\" : \"value2\" }}");

    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "test1", strlen ("test1"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_string (context_keyval, "value1", strlen ("value1"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "test1", strlen ("test1"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_value_string (context_keyval, "value2", strlen ("value2"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_config_finalize (&context_config, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_TRUE (writer->marshal (config, json) == DPLUGIN_STATUS_OK);

    ASSERT_JSON_STREQ (expected.c_str (), json.c_str ());
}

TEST_F (MarshallConfigTest, duplicate_sections_shall_succeed)
{
    std::string json;
    std::string expected;

    formatJson (expected,
            "{\"version\" : \"1.0.0\", \
                \"config\" : {          \
                \"section_name\" :  { }, \
                \"section_name@2\" : {}}}");

    status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_section, "section_name", strlen ("section_name"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_section);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_begin (context_config, DISIR_CONTEXT_SECTION, &context_section);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_section, "section_name", strlen ("section_name"));
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_finalize (&context_section);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_config_finalize (&context_config, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_TRUE (writer->marshal (config, json) == DPLUGIN_STATUS_OK);

    ASSERT_JSON_STREQ (expected.c_str (), json.c_str ());
}

TEST_F (MarshallConfigTest, context_without_value_shall_succeed)
{
    std::string json;
    std::string expected;

    ASSERT_TRUE ( formatJson (expected,
            "{\"version\" : \"1.0.0\", \
                \"config\" : {          \
                \"fake_name\" : \"UNKNOWN\"}}"));

    status = dc_begin (context_config, DISIR_CONTEXT_KEYVAL, &context_keyval);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_set_name (context_keyval, "fake_name", strlen ("fake_name"));
    EXPECT_STATUS (DISIR_STATUS_NOT_EXIST, status);

    status = dc_finalize (&context_keyval);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    status = dc_config_finalize (&context_config, &config);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    auto pstatus = writer->marshal (config, json);
    ASSERT_EQ (DPLUGIN_STATUS_OK, pstatus);

    dc_putcontext (&context_keyval);

    ASSERT_JSON_STREQ (expected.c_str (), json.c_str ());
}

