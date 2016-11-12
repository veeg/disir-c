#include <gtest/gtest.h>

#include <disir/disir.h>
#include "tinytoml/toml.h"
#include <disir/fslib/toml.h>

#include "test_helper.h"


class TOMLConfigTest : public testing::DisirTestTestPlugin
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
};

TEST_F (TOMLConfigTest, serialize_unserialize_compare)
{
    ASSERT_NO_FATAL_FAILURE (ConfigSerializeUnserializeCompareAll ());
}

