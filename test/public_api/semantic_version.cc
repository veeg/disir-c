#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>
#include <disir/util.h>

#include "test_helper.h"


class SemanticVersionTest : public testing::Test
{
    void SetUp()
    {
        memset (&sv1, 0, sizeof (struct semantic_version));
        memset (&sv2, 0, sizeof (struct semantic_version));

        sv1.sv_major = sv2.sv_major = 3;
        sv1.sv_minor = sv2.sv_minor = 6;
        sv1.sv_patch = sv2.sv_patch = 76;
    }

    void TearDown()
    {
    }

public:
    int res;
    struct semantic_version sv1;
    struct semantic_version sv2;
};

TEST_F (SemanticVersionTest, compare_equal)
{
    res = dc_semantic_version_compare (&sv1, &sv2);
    ASSERT_EQ (res, 0);
}

TEST_F (SemanticVersionTest, compare_major)
{
    sv1.sv_major = 5;
    res = dc_semantic_version_compare (&sv1, &sv2);
    EXPECT_GT (res, 0);

    sv2.sv_major = 6;
    res = dc_semantic_version_compare (&sv1, &sv2);
    EXPECT_LT (res, 0);
}

TEST_F (SemanticVersionTest, compare_minor)
{
    sv1.sv_minor = 7;
    res = dc_semantic_version_compare (&sv1, &sv2);
    EXPECT_GT (res, 0);

    sv2.sv_minor = 8;
    res = dc_semantic_version_compare (&sv1, &sv2);
    EXPECT_LT (res, 0);
}

TEST_F (SemanticVersionTest, compare_patch)
{
    sv1.sv_patch = 78;
    res = dc_semantic_version_compare (&sv1, &sv2);
    EXPECT_GT (res, 0);

    sv2.sv_patch = 90;
    res = dc_semantic_version_compare (&sv1, &sv2);
    EXPECT_LT (res, 0);
}

