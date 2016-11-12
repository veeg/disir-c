
#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include <cstdio>

#include <disir/disir.h>
#include "tinytoml/toml.h"
#include <disir/fslib/toml.h>

#include "test_helper.h"


class TomlUnserializeConfigTest : public testing::DisirTestTestPlugin
{
public:
    void SetUp ()
    {
        DisirTestTestPlugin::SetUp ();

        DisirLogTestBodyEnter ();
    }

    void TearDown ()
    {
        DisirLogTestBodyExit ();

        DisirTestTestPlugin::TearDown ();
    }

    void setup_testconfig (const char *entry)
    {
        std::stringstream filepath;
        filepath << CMAKE_SOURCE_DIRECTORY << "/test/plugins/toml/testdata/" << entry << ".toml";

        // XXX: Construct full filename to testdata
        input = fopen (filepath.str().c_str(), "r");
        ASSERT_TRUE (input != NULL);

        status = disir_mold_read (instance, entry, &mold);
        ASSERT_STATUS (DISIR_STATUS_OK, status);

        status = dio_toml_unserialize_config (instance, input, mold, &config);
        ASSERT_STATUS (DISIR_STATUS_OK, status);
    }

public:
    enum disir_status status = DISIR_STATUS_OK;
    struct disir_config *config = NULL;
    struct disir_mold *mold= NULL;
    FILE *input = NULL;
};

TEST_F (TomlUnserializeConfigTest, basic_keyval)
{
    setup_testconfig ("basic_keyval");

    // Lets start querying the keyval
}

