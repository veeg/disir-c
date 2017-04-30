#include "test_helper.h"

#include <disir/fslib/util.h>
#include <disir/fslib/toml.h>
#include <disir/fslib/json.h>

#include "log.h"

const char *molds[] = {
    "basic_keyval",
    "basic_section",
    "json_test_mold",
    "restriction_keyval_numeric_types",
    "restriction_entries",
    "restriction_config_parent_keyval_min_entry",
    "restriction_config_parent_keyval_max_entry",
    "restriction_config_parent_section_max_entry",
    "restriction_section_parent_keyval_max_entry",
    "basic_version_difference",
    "complex_section",
    "config_query_permutations",
};

class SerializeUnserializeTest : public ::testing::DisirTestTestPlugin,
                                 public ::testing::WithParamInterface<const char *>
{
public:
    void serialize_unserialize_mold (const char *entry,
                                     const char *suffix,
                                     dio_serialize_mold func_serialize,
                                     dio_unserialize_mold func_unserialize)
    {
        struct disir_mold *mold_original = NULL;
        struct disir_mold *mold_parsed = NULL;
        struct disir_context *context_mold1 = NULL;
        struct disir_context *context_mold2 = NULL;
        FILE *file = NULL;

        log_test ("SerializeUnserialize mold %s", entry);

        status = disir_mold_read (instance, "test", entry, &mold_original);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        if (status != DISIR_STATUS_OK)
        {
            log_test ("mold_read failed: %s", disir_status_string (status));
            goto out;
        }

        char filepath[4098];
        snprintf (filepath, 4098, "/tmp/disir_plugin_serialize_unserialize_%s.%s", entry, suffix);
        file = fopen (filepath, "w+");
        EXPECT_TRUE (file != NULL);
        if (file == NULL)
        {
            log_test ("Failed to open file...");
            goto out;
        }
        status = func_serialize (instance, mold_original, file);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        if (status != DISIR_STATUS_OK)
            goto out;

        fseek (file, 0, SEEK_SET);

        status = func_unserialize (instance, file, &mold_parsed);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        if (status != DISIR_STATUS_OK)
            goto out;

        context_mold1 = dc_mold_getcontext (mold_original);
        context_mold2 = dc_mold_getcontext (mold_parsed);
        EXPECT_TRUE (context_mold1 != NULL);
        EXPECT_TRUE (context_mold2 != NULL);
        if (context_mold1 == NULL || context_mold2 == NULL)
            goto out;

        status = dc_compare (context_mold1, context_mold2, NULL);
        EXPECT_STATUS (DISIR_STATUS_OK, status)
    out:
        log_test ("compare serialize-unserialize mold out clause");
        // Cleanup this entry and move to the next
        if (file != NULL)
        {
            fclose (file);
        }
        dc_putcontext (&context_mold1);
        dc_putcontext (&context_mold2);
        disir_mold_finished (&mold_original);
        disir_mold_finished (&mold_parsed);
    }

    void serialize_unserialize_config (const char *entry,
                                       dio_serialize_config func_serialize,
                                       dio_unserialize_config func_unserialize)
    {
        struct disir_config *config_original = NULL;
        struct disir_config *config_parsed = NULL;
        struct disir_context *context_config1 = NULL;
        struct disir_context *context_config2 = NULL;
        FILE *file = NULL;
        struct disir_mold *mold = NULL;

        log_test ("SerializeUnserialize %s", entry);

        // read the current entry
        status = disir_config_read (instance, "test", entry,
                                    NULL, &config_original);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        if (status != DISIR_STATUS_OK)
        {
            log_test ("config_read failed: %s", disir_status_string (status));
            goto out;
        }

        // open file for reading and writing
        char filepath[4098];
        snprintf (filepath, 4098, "/tmp/disir_plugin_serialize_unserialize_%s.toml", entry);
        file = fopen (filepath, "w+");
        EXPECT_TRUE (file != NULL);
        if (file == NULL)
        {
            log_test ("Failed to open file...");
            goto out;
        }

        // Serialize the current entry config
        status = func_serialize (instance, config_original, file);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        if (status != DISIR_STATUS_OK)
            goto out;

        // Unserialize the previously serialized config
        // XXX: Cheat by extracting the mold directly from the config.
        mold = config_original->cf_mold;
        fseek (file, 0, SEEK_SET);
        status = func_unserialize (instance, file, mold, &config_parsed);
        EXPECT_STATUS (DISIR_STATUS_OK, status);
        if (status != DISIR_STATUS_OK)
            goto out;

        // compare the two
        context_config1 = dc_config_getcontext (config_original);
        context_config2 = dc_config_getcontext (config_parsed);
        EXPECT_TRUE (context_config1 != NULL);
        EXPECT_TRUE (context_config2 != NULL);
        if (context_config1 == NULL || context_config2 == NULL)
            goto out;
        status = dc_compare (context_config1, context_config2, NULL);
        EXPECT_STATUS (DISIR_STATUS_OK, status)

    out:
        log_test ("compare serialize-unserialize out clause");
        // Cleanup this entry and move to the next
        if (file != NULL)
        {
            fclose (file);
        }
        dc_putcontext (&context_config1);
        dc_putcontext (&context_config2);
        disir_config_finished (&config_original);
        disir_config_finished (&config_parsed);
    }
};

TEST_P(SerializeUnserializeTest, toml)
{
    const char *key= GetParam();

    ASSERT_NO_FATAL_FAILURE (
        serialize_unserialize_config (key, dio_toml_serialize_config, dio_toml_unserialize_config);
    );
}

TEST_P(SerializeUnserializeTest, config_json)
{
    const char *key= GetParam();

    ASSERT_NO_FATAL_FAILURE (
        serialize_unserialize_config (key, dio_json_serialize_config, dio_json_unserialize_config);
    );
}

TEST_P(SerializeUnserializeTest, mold_json)
{
    const char *key= GetParam();

    ASSERT_NO_FATAL_FAILURE (
        serialize_unserialize_mold (key, "json", dio_json_serialize_mold, dio_json_unserialize_mold);
    );
}

INSTANTIATE_TEST_CASE_P(MoldKey, SerializeUnserializeTest, ::testing::ValuesIn(molds));

