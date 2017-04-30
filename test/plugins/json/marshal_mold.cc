#include "test_json.h"
#include "json/output.h"


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
        struct disir_mold *mold = NULL;
        dio::MoldWriter *writer = NULL;
};

TEST_F (MarshallMoldTest, marshal_mold_shall_match)
{
  std::string json, expected_json;
  Json::Value expected;

  ASSERT_TRUE (GetJsonMold (expected));
  expected_json = getJsonString (expected);
  status = writer->marshal (mold, json);
  ASSERT_TRUE (compare_json_objects (expected_json, json));
  //ASSERT_JSON_STREQ (expected_json.c_str (), json.c_str ());
  EXPECT_STATUS (DISIR_STATUS_OK, status);
}

TEST_F (MarshallMoldTest, invalid_mold_shall_fail)
{
    struct disir_mold *invalid_mold;
    std::string json;

    invalid_mold = NULL;

    EXPECT_STATUS (DISIR_STATUS_INTERNAL_ERROR, writer->marshal (invalid_mold, json));
}

TEST_F (MarshallMoldTest, DISABLED_tets)
{

}
