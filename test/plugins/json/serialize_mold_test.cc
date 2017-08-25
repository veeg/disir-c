#include "test_json.h"
#include "json/json_serialize.h"


class MarshallMoldTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
        DisirLogCurrentTestEnter();

        status = disir_mold_read (instance, "test", "json_test_mold", &mold);
        ASSERT_TRUE (mold != NULL);

        writer = new dio::MoldWriter (instance);

        DisirLogTestBodyEnter();
    }

    void TearDown()
    {
        DisirLogTestBodyExit();

        if (mold)
        {
            disir_mold_finished (&mold);
        }

        if (writer)
        {
            delete writer;
        }

        DisirLogCurrentTestExit();
    }

public:
    void
    compare_reference_and_serialized (const char* reference_name, const char *serialized)
    {
        std::stringstream ss;
        std::ifstream reference (m_json_references_path + reference_name);
        ASSERT_TRUE (reference.is_open());

        ss << reference.rdbuf();

        ASSERT_STREQ (ss.str().c_str(), serialized);
    }

public:
    enum disir_status status;
    struct disir_mold *mold = NULL;
    dio::MoldWriter *writer = NULL;
    Json::StyledWriter json_writer;
    std::string string;
};

TEST_F (MarshallMoldTest, marshal_mold_shall_match)
{
    status = writer->serialize (mold, string);
    ASSERT_STATUS (DISIR_STATUS_OK, status);

    ASSERT_NO_FATAL_FAILURE (
        compare_reference_and_serialized ("json_test_mold.json", string.c_str());
    );
}

TEST_F (MarshallMoldTest, marshal_invalid_argument)
{
    status = writer->serialize (NULL, string);
    ASSERT_STATUS (DISIR_STATUS_INVALID_ARGUMENT, status);
}

