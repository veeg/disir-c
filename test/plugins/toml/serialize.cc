
#include <gtest/gtest.h>
#include <sstream>
#include <fstream>

#include <disir/disir.h>
#include "tinytoml/toml.h"
#include <disir/fslib/toml.h>
#include "fdstream.hpp"

#include "test_helper.h"


class TOMLSerializeConfigTest : public testing::DisirTestTestPlugin
{
public:
    void SetUp ()
    {
        DisirTestTestPlugin::SetUp ();

        serial_fd = fopen ("/tmp/disir_plugin_toml_serialize_config_text.txt", "w+");
        ASSERT_TRUE (serial_fd != NULL);

        DisirLogTestBodyEnter ();
    }

    void TearDown ()
    {
        DisirLogTestBodyExit ();

        DisirTestTestPlugin::TearDown ();
    }

    void setup_testconfig (const char *entry, struct disir_version *semver)
    {
        status = disir_mold_read (instance, "test", entry, &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = disir_generate_config_from_mold (mold, semver, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

    void compare_serial_with_file (const char *filename)
    {
        std::stringstream filepath;
        filepath << CMAKE_SOURCE_DIRECTORY << "/test/plugins/toml/testdata/" << filename;
        std::cout << "FILEPATH!: " << filepath.str () << std::endl;

        // Read origin file
        std::ifstream origin_file (filepath.str (), std::ifstream::in);
        ASSERT_TRUE (origin_file.is_open ());

        // Get a proper istream for the written serial file
        fseek (serial_fd, 0, SEEK_SET);
        boost::fdistream serial_file(fileno(serial_fd));

        // Get the strings and compare them.
        origin_ss << origin_file.rdbuf ();
        serial_ss << serial_file.rdbuf ();
        ASSERT_STREQ (origin_ss.str ().c_str (), serial_ss.str ().c_str ());
    }

public:
    enum disir_status status = DISIR_STATUS_OK;
    struct disir_config *config = NULL;
    struct disir_mold *mold = NULL;
    std::stringstream serial_ss;
    std::stringstream origin_ss;
    FILE *serial_fd;
};

TEST_F (TOMLSerializeConfigTest, DISABLED_basic_keyval)
{
    ASSERT_NO_FATAL_FAILURE (setup_testconfig ("basic_keyval", NULL));

    dio_toml_serialize_config (instance, config, serial_fd);
    compare_serial_with_file ("basic_keyval.toml");
}

TEST_F (TOMLSerializeConfigTest, DISABLED_basic_section)
{
    ASSERT_NO_FATAL_FAILURE (setup_testconfig ("basic_section", NULL));

    dio_toml_serialize_config (instance, config, serial_fd);
    compare_serial_with_file ("basic_section.toml");
}

TEST_F (TOMLSerializeConfigTest, DISABLED_complex_section)
{
    ASSERT_NO_FATAL_FAILURE (setup_testconfig ("complex_section", NULL));

    dio_toml_serialize_config (instance, config, serial_fd);
    compare_serial_with_file ("complex_section.toml");
}

