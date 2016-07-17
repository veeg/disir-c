#include <gtest/gtest.h>

// PUBLIC API
#include <disir/disir.h>

#include "test_helper.h"


class DisirInstanceTest : public testing::DisirTestWrapper
{
    void SetUp()
    {
        status = DISIR_STATUS_OK;
        disir = NULL;

        DisirLogTestBodyEnter ();
    }

    void TearDown()
    {
        DisirLogTestBodyExit ();
    }

public:
    enum disir_status status;
    struct disir_instance *disir;
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

