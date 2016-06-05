#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class DisirInstanceTest : public testing::Test
{
    void SetUp()
    {
        status = DISIR_STATUS_OK;
        disir = NULL;
    }

    void TearDown()
    {
    }

public:
    enum disir_status status;
    struct disir *disir;
};

TEST_F (DisirInstanceTest, should_create_destroy_disir_instance)
{
    status = disir_instance_create (NULL, NULL, &disir);
    ASSERT_EQ (DISIR_STATUS_OK, status);
    ASSERT_TRUE (disir != NULL);

    status = disir_instance_destroy (&disir);
    ASSERT_EQ (DISIR_STATUS_OK, status);
    ASSERT_TRUE (disir == NULL);
}

