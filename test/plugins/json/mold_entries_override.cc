// JSON local
#include "test_json.h"
#include "json/json_serialize.h"
#include "json/json_unserialize.h"

// disir
#include <disir/fslib/util.h>

// standard
#include <fstream>
#include <experimental/filesystem>

const std::string molds[] = {
    "basic_keyval",
    "json_test_mold",
    "complex_section",
    "restriction_config_parent_keyval_min_entry",
};

const std::vector<std::tuple<std::string, std::string>> override_entries = {
    std::make_tuple ("basic_keyval","basic_keyval_override"),
    std::make_tuple ("json_test_mold", "json_test_mold_override"),
    std::make_tuple ("complex_section","complex_section_override_equal_version"),
    std::make_tuple ("restriction_config_parent_keyval_min_entry",
                     "restriction_config_parent_keyval_min_entry_override"),
};

class MoldOverrideParameterized :
    public testing::JsonDioTestWrapper ,
    public ::testing::WithParamInterface<std::tuple<std::string,
                                                    std::tuple<std::string, std::string>>>
{
    void SetUp ()
    {
        DisirLogCurrentTestEnter();

        read_override_mold_references ();

        DisirLogTestBodyEnter();
    }

    void TearDown ()
    {
        // Remove all molds in fs
        std::experimental::filesystem::remove_all (m_mold_base_dir);
        teardown_override_mold_references ();
    }

    public:
        void
        compare_override_and_reference (std::string entry_mold,
                                        std::string entry_override_namespace,
                                        std::string entry_mold_override)
        {
            struct disir_mold *mold_override = NULL;
            struct disir_mold *mold_reference = NULL;
            struct disir_context *context_override = NULL;
            struct disir_context *context_reference = NULL;
            std::string entry_mold_namespace (entry_mold + "/" + "__namespace");
            std::string entry_mold_override_id (entry_override_namespace + "/" +
                                                entry_mold_override);

            // retrieve override mold reference
            mold_reference = m_override_reference_molds[entry_mold.c_str()];
            context_reference = dc_mold_getcontext (mold_reference);

            status = disir_mold_read (instance, "test", entry_mold.c_str(), &mold_override);
            ASSERT_STATUS (DISIR_STATUS_OK, status);

            status = disir_mold_write (instance, "json_test",
                                       entry_mold_namespace.c_str(), mold_override);
            ASSERT_STATUS (DISIR_STATUS_OK, status);

            disir_mold_finished (&mold_override);

            // Copy mold override file into mold_base_id/namespace
            emplace_mold (entry_override_namespace.c_str(), entry_mold_override.c_str());

            status = disir_mold_read (instance, "json_test",
                                      entry_mold_override_id.c_str(), &mold_override);
            if (entry_mold_override.find (entry_mold) != std::string::npos)
            {
                ASSERT_STATUS (DISIR_STATUS_OK, status);

                context_override = dc_mold_getcontext (mold_override);
                ASSERT_NE (nullptr, context_override);

                status = dc_compare (context_override, context_reference, NULL);
                ASSERT_STATUS (DISIR_STATUS_OK, status);
            }
            else
            {
                char error[4096];
                ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
                snprintf (error, 4096, "No plugin in group 'json_test' contains mold entry '%s'",
                                       entry_mold_override_id.c_str());
                ASSERT_STREQ (error, disir_error (instance));
            }

            if (context_reference)
                dc_putcontext (&context_reference);
            if (context_override)
                dc_putcontext (&context_override);
            if (mold_override)
                disir_mold_finished (&mold_override);
        }
    public:
        dio::MoldReader *reader = NULL;
        dio::MoldWriter *writer = NULL;
        enum disir_status status;
};

class MoldOverrideTest : public testing::JsonDioTestWrapper
{
    void SetUp ()
    {
        DisirLogCurrentTestEnter();

        reader = new dio::MoldReader (instance);
        writer = new dio::MoldWriter (instance);

        GetJsonObject (root, m_override_entries_path + "json_test_mold_override.json");

        status = disir_mold_read (instance, "test", "json_test_mold", &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = writer->serialize (mold, mold_json);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        disir_mold_finished (&mold);

        DisirLogTestBodyEnter();
    }

    void TearDown ()
    {
        delete reader;
        delete writer;

        if (mold)
        {
            disir_mold_finished (&mold);
        }
    }

    public:
        Json::Value root;
        dio::MoldReader *reader;
        dio::MoldWriter *writer;
        Json::StyledWriter jsonWriter;
        std::stringstream mold_json;
        enum disir_status status;
        struct disir_mold *mold = NULL;

};

TEST_F (MoldOverrideTest, invalid_override_on_uncovered_version_sync)
{
    ASSERT_NO_THROW (
        root["sync"][0]["override"] = "3.0";
    );

    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->is_override_mold_entry (json_stream);
    ASSERT_STATUS (DISIR_STATUS_FS_ERROR, status);

    ASSERT_STREQ ("invalid mold override entry: (section_name.k1) "
                  "version not accounted for in sync mapping",
                  disir_error (instance)
    );
}

TEST_F (MoldOverrideTest, invalid_override_on_wrong_namespace_type)
{
    ASSERT_NO_THROW (
        root["sync"][0]["override"] = 123;
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->is_override_mold_entry (json_stream);
    ASSERT_STATUS (DISIR_STATUS_FS_ERROR, status);

    ASSERT_STREQ ("invalid sync entry: sync entry (0) override version not string",
                  disir_error (instance));
}

TEST_F (MoldOverrideTest, error_if_no_override_entry_exists)
{
    ASSERT_NO_THROW (
        root.clear ();
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->is_override_mold_entry (json_stream);
    ASSERT_STATUS (DISIR_STATUS_NOT_EXIST, status);
}

TEST_F (MoldOverrideTest, invalid_override_on_absent_override_version)
{
    ASSERT_NO_THROW (
        root["sync"][0]["override"] = Json::nullValue;
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->is_override_mold_entry (json_stream);
    ASSERT_STATUS (DISIR_STATUS_FS_ERROR, status);

    ASSERT_STREQ ("invalid sync entry: sync entry (0) missing override version",
                  disir_error (instance));
}

TEST_F (MoldOverrideTest, invalid_override_on_absent_namespace_version)
{
    ASSERT_NO_THROW (
        root["sync"][0]["namespace"] = Json::nullValue;
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->is_override_mold_entry (json_stream);
    ASSERT_STATUS (DISIR_STATUS_FS_ERROR, status);

    ASSERT_STREQ ("invalid sync entry: sync entry (0) missing namespace version",
                  disir_error (instance));
}

TEST_F (MoldOverrideTest, invalid_override_on_absent_keyval_override_value)
{
    ASSERT_NO_THROW (
        root["override"]["section_name.k1"][0]["value"] = Json::nullValue;
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->is_override_mold_entry (json_stream);
    ASSERT_STATUS (DISIR_STATUS_FS_ERROR, status);

    ASSERT_STREQ ("invalid mold override entry: (section_name.k1) missing value",
                  disir_error (instance));
}

TEST_F (MoldOverrideTest, invalid_override_on_absent_keyval_override_version)
{
    ASSERT_NO_THROW (
        root["override"]["section_name.k1"][0]["version"] = Json::nullValue;
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->is_override_mold_entry (json_stream);
    ASSERT_STATUS (DISIR_STATUS_FS_ERROR, status);

    ASSERT_STREQ ("invalid mold override entry: (section_name.k1) missing version",
                  disir_error (instance));
}

TEST_F (MoldOverrideTest, invalid_override_on_higher_override_version_than_mold_version)
{
    ASSERT_NO_THROW (
        root["sync"][0]["namespace"] = "3.0"
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->set_mold_override (json_stream);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = reader->unserialize (mold_json, &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 1);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD",
                                  "keyval 'section_name.k1' override version (3.0) "
                                  "is higher than namespace mold version (2.0)");
}

TEST_F (MoldOverrideTest, invalid_override_on_override_type_mismatch_existing_default)
{
    ASSERT_NO_THROW (
        root["override"]["section_name.k1"][1]["value"] = 1;
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->set_mold_override (json_stream);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = reader->unserialize (mold_json, &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 1);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD",
                                  "override type mismatch on keyval 'section_name.k1'. "
                                  "Should be STRING");
}

TEST_F (MoldOverrideTest, invalid_override_on_override_type_mismatch_new_default)
{
    ASSERT_NO_THROW (
        root["override"]["section_name.k1"][0]["value"] = 1;
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->set_mold_override (json_stream);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = reader->unserialize (mold_json, &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 1);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD",
                                  "override type mismatch on keyval 'section_name.k1'. "
                                  "Should be STRING");
}

TEST_F (MoldOverrideTest, invalid_override_on_non_existing_keyval_in_namespace_mold)
{
    ASSERT_NO_THROW (
        root["override"]["absent_keyval"]["value"] = 1;
        root["override"]["absent_keyval"]["version"] = "1.0";
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->set_mold_override (json_stream);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = reader->unserialize (mold_json, &mold);
    ASSERT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    ASSERT_INVALID_CONTEXT_COUNT (mold, 1);
    ASSERT_INVALID_CONTEXT_EXIST (mold, NULL, "MOLD",
                                  "override mold keyval 'absent_keyval' "
                                  "does not exist in namespace mold");
}

TEST_F (MoldOverrideTest, override_value_int_as_float_shall_succeed)
{
    ASSERT_NO_THROW (
        root["override"]["section_name.float"]["value"] = 16;
    );
    std::stringstream json_stream (jsonWriter.writeOrdered (root));

    status = reader->set_mold_override (json_stream);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    status = reader->unserialize (mold_json, &mold);
    ASSERT_STATUS (DISIR_STATUS_OK, status);
}

TEST_P (MoldOverrideParameterized, mold_override_unserialize)
{
    auto entry = GetParam ();
    auto override_entry = std::get<1>(entry);

    ASSERT_NO_FATAL_FAILURE (
        compare_override_and_reference (std::get<0>(entry),
                                        std::get<0>(override_entry),
                                        std::get<1>(override_entry));
    );
}

INSTANTIATE_TEST_CASE_P(MoldOverrideKey,
                        MoldOverrideParameterized,
                        ::testing::Combine (::testing::ValuesIn(molds),
                                            ::testing::ValuesIn(override_entries)));

