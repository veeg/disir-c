#include "test_json.h"
#include "json/json_serialize.h"
#include "json/json_unserialize.h"
#include <tuple>
#include <set>

class UnMarshallMoldTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        reader = new dio::MoldReader (instance);
        writer = new dio::MoldWriter (instance);

        status = disir_mold_read (instance, "test", "json_test_mold", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        std::string json_mold_string;
        Json::Reader json_reader;

        status = writer->serialize (mold, json_mold_string);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        bool success = json_reader.parse (json_mold_string, json_mold);
        ASSERT_TRUE (success);

        disir_mold_finished (&mold);

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
        Json::Value json_mold;
        Json::StyledWriter json_writer;
        struct disir_context *context_config = NULL;
        struct disir_config *config = NULL;
        struct disir_context *context_mold = NULL;
        dio::MoldReader *reader = NULL;
        dio::MoldWriter *writer = NULL;
};

TEST_F (UnMarshallMoldTest, read_mold_shall_succeed)
{
    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (UnMarshallMoldTest, invalid_context_on_missing_defaults)
{
    ASSERT_NO_THROW (
        Json::Value& keyval = json_mold["mold"]["test1"];
        keyval["defaults"] = Json::nullValue;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL", "Missing default entries");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_defaults_type)
{
    ASSERT_NO_THROW (
        Json::Value& keyval = json_mold["mold"]["test1"];
        keyval["defaults"] = 1;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL", "Defaults should be of type array");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_missing_default_value)
{
    ASSERT_NO_THROW (
        Json::Value& def = json_mold["mold"]["test1"]["defaults"];
        def[0]["value"] = Json::nullValue;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL", "Default value is not present");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_documentation_type)
{
    ASSERT_NO_THROW (
        Json::Value& doc = json_mold["mold"]["test1"]["documentation"];
        doc = 1;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL", "Documentation shall be of type string");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_absent_keyval_type)
{
    ASSERT_NO_THROW (
        json_mold["mold"]["test1"]["type"] = Json::nullValue;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL", "Keyval type is not present");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_keyval_type)
{
    ASSERT_NO_THROW (
        json_mold["mold"]["test1"]["type"] = 1;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL",
                                        "Keyval type is not of type string");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_deprecated_type)
{
    ASSERT_NO_THROW (
        json_mold["mold"]["test1"]["deprecated"] = 1;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL",
        "property 'deprecated' expects JSON type string");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_deprecated_format)
{
    ASSERT_NO_THROW (
        json_mold["mold"]["test1"]["deprecated"] = "wrong_format";
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL",
        "property 'deprecated' version incorrectly formated");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_elements_type)
{
    ASSERT_NO_THROW (
        json_mold["mold"]["section_name"]["elements"] = "wrong_type";
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "section_name", "SECTION",
                                        "Section elements must be of type object");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_ements_type)
{
    ASSERT_NO_THROW (
        json_mold["mold"]["section_name"]["elements"] = "wrong_type";
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "section_name", "SECTION",
                                        "Section elements must be of type object");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_keyval_nor_section)
{
    ASSERT_NO_THROW (
        json_mold["mold"]["section_name"] = Json::objectValue;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 1);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD",
                                        "Could not resolve whether child object 'section_name'"\
                                        " is of type keyval or section");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_restriction_object)
{
    ASSERT_NO_THROW (
        json_mold["mold"]["test1"]["restrictions"] = "wrong_type";
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL",
                                        "Restrictions is not of type array");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_restriction_type)
{
    ASSERT_NO_THROW (
        Json::Value& restrictions = json_mold["mold"]["test1"]["restrictions"];
        restrictions[0]["type"] = "invalid_type";
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL",
                                        "Unknown restriction type: invalid_type");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_absent_restriction_value)
{
    ASSERT_NO_THROW (
        Json::Value& restrictions = json_mold["mold"]["test1"]["restrictions"];
        restrictions[0]["value"] = Json::nullValue;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL",
                                        "No restriction value present");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_restriction_value_range_type)
{
    ASSERT_NO_THROW (
        Json::Value& restrictions = json_mold["mold"]["float"]["restrictions"];
        restrictions[0]["value"] = 1;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);


    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "float", "KEYVAL",
                                        "restriction range value field should be array");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_range_value_count)
{
    ASSERT_NO_THROW (
        Json::Value& restrictions = json_mold["mold"]["float"]["restrictions"];
        restrictions[0]["value"].append (1);
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);


    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "float", "KEYVAL",
                                        "restriction range value field should have 2 values"\
                                        ", got 3");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_range_value_type)
{
    ASSERT_NO_THROW (
        Json::Value& restrictions = json_mold["mold"]["float"]["restrictions"];
        restrictions[0]["value"] = Json::arrayValue;
        restrictions[0]["value"].append ("1");
        restrictions[0]["value"].append ("2");
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);


    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "float", "KEYVAL",
                                        "wrong type for value range - got string, expected"\
                                        " integer or float");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_value_range_min_larger_than_max)
{
    ASSERT_NO_THROW (
        Json::Value& restrictions = json_mold["mold"]["float"]["restrictions"];
        restrictions[0]["value"] = Json::arrayValue;
        restrictions[0]["value"].append (10.1);
        restrictions[0]["value"].append (1.2);
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);


    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "float", "KEYVAL",
                                        "restriction value range should have min < max ("\
                                        "10.1, 1.2)");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_absent_restriction_value_min_type)
{
    ASSERT_NO_THROW (
        Json::Value& restrictions = json_mold["mold"]["integer"]["restrictions"];
        restrictions[0]["type"] = "invalid";
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "integer", "KEYVAL",
                                        "Unknown restriction type: invalid");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_restriction_numeric_value_wrong_object)
{
    ASSERT_NO_THROW (
        Json::Value& restrictions = json_mold["mold"]["integer"]["restrictions"];
        restrictions[0]["value"] = "invalid";
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "integer", "KEYVAL",
        "restriction numeric value field should be integer or float, found string");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_unresolvable_top_level_type)
{
    ASSERT_NO_THROW (
        json_mold["mold"]["section_name"] = Json::nullValue;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 1);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD",
                                        "Could not resolve whether child object 'section_name'"\
                                        " is of type keyval or section");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_section_elements_wrong_type)
{
    ASSERT_NO_THROW (
        json_mold["mold"]["section_name"]["elements"] = "invalid type";
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "section_name", "SECTION",
                                        "Section elements must be of type object");
}

TEST_F (UnMarshallMoldTest, invalid_context_on_error_override_missing_defaults)
{
    ASSERT_NO_THROW (
        Json::Value& restrictions = json_mold["mold"]["test1"]["restrictions"];
        restrictions[0]["value"] = Json::nullValue;
        json_mold["mold"]["test1"]["defaults"] = Json::nullValue;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "test1", "KEYVAL",
                                        "Missing default entries");
}

TEST_F (UnMarshallMoldTest, unserialize_value_int_as_float_shall_succeed)
{
    ASSERT_NO_THROW (
        Json::Value& defaults = json_mold["mold"]["float"]["defaults"];
        defaults[0]["value"] = 12;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (UnMarshallMoldTest, invalid_context_on_wrong_default_value_type)
{
    ASSERT_NO_THROW (
        Json::Value& defaults = json_mold["mold"]["float"]["defaults"];
        defaults[0]["value"] = "wrong_type";
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD", NULL);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "float", "KEYVAL",
                                        "value default should be FLOAT, got string");
}

TEST_F (UnMarshallMoldTest, default_introduced_wrong_json_type)
{
    ASSERT_NO_THROW (
        Json::Value& defaults = json_mold["mold"]["float"]["defaults"];
        defaults[0]["introduced"] = 1.10;
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "float", "KEYVAL",
                                        "property 'introduced' expects JSON type string");
}

TEST_F (UnMarshallMoldTest, default_introduced_incorrectly_formated)
{
    ASSERT_NO_THROW (
        Json::Value& defaults = json_mold["mold"]["float"]["defaults"];
        defaults[0]["introduced"] = "1.x";
    );

    status = reader->unserialize (json_writer.writeOrdered (json_mold), &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 2);
    ASSERT_INVALID_CONTEXT_EXIST (mold, "float", "KEYVAL",
                                        "property 'introduced' version incorrectly formated");
}
