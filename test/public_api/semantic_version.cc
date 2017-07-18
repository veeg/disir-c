#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>
#include <disir/util.h>

#include "test_helper.h"


class SemanticVersionTest : public testing::Test
{
    void SetUp()
    {
        memset (&sv1, 0, sizeof (struct disir_version));
        memset (&sv2, 0, sizeof (struct disir_version));

        sv1.sv_major = sv2.sv_major = 3;
        sv1.sv_minor = sv2.sv_minor = 6;
    }

    void TearDown()
    {
    }

public:
    int res;
    struct disir_version sv1;
    struct disir_version sv2;
};

TEST_F (SemanticVersionTest, compare_equal)
{
    res = dc_version_compare (&sv1, &sv2);
    ASSERT_EQ (res, 0);
}

TEST_F (SemanticVersionTest, compare_major)
{
    sv1.sv_major = 5;
    res = dc_version_compare (&sv1, &sv2);
    EXPECT_GT (res, 0);

    sv2.sv_major = 6;
    res = dc_version_compare (&sv1, &sv2);
    EXPECT_LT (res, 0);
}

TEST_F (SemanticVersionTest, compare_minor)
{
    sv1.sv_minor = 7;
    res = dc_version_compare (&sv1, &sv2);
    EXPECT_GT (res, 0);

    sv2.sv_minor = 8;
    res = dc_version_compare (&sv1, &sv2);
    EXPECT_LT (res, 0);
}

TEST_F (SemanticVersionTest, set_invalid)
{
    // Should basically not crash. Thats the test
    dc_version_set (NULL, &sv2);
    dc_version_set (&sv1, NULL);
}

TEST_F (SemanticVersionTest, set_valid)
{
    // Should basically not crash. Thats the test
    memset (&sv1, 0, sizeof (struct disir_version));

    dc_version_set (&sv1, &sv2);

    EXPECT_EQ (sv1.sv_major, sv2.sv_major);
    EXPECT_EQ (sv1.sv_minor, sv2.sv_minor);
}


