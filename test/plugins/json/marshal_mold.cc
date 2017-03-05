#include <gtest/gtest.h>
#include <disir/disir.h>
#include <json/json.h>
#include "test_helper.h"
#include "log.h"
#include "output.h"


class MarshallMoldTest : public testing::JsonDioTestWrapper
{
    void SetUp()
    {
       SetUpDisir ();

       mold = NULL;

       status = disir_mold_input (disir, "test", "json_test_mold", &mold);
       ASSERT_TRUE (mold != NULL);

       writer = new dio::MoldWriter (disir);

   }
    void TearDown()
    {
        if (mold)
        {
            disir_mold_finished (&mold);
        }

        if (writer)
        {
            delete writer;
        }

        TearDownDisir ();
    }

    public:
        struct disir_mold *mold;
        dio::MoldWriter *writer;
};

TEST_F (MarshallMoldTest, marshal_mold_shall_match)
{
  std::string json, expected_json;
  enum dplugin_status status;
  Json::Value expected;

  ASSERT_TRUE (GetJsonMold (expected));
  expected_json = getJsonString (expected);
  status = writer->marshal (mold, json);
  ASSERT_TRUE (compare_json_objects (expected_json, json));
  //ASSERT_JSON_STREQ (expected_json.c_str (), json.c_str ());
  ASSERT_TRUE (status == DPLUGIN_STATUS_OK);
}

TEST_F (MarshallMoldTest, invalid_mold_shall_fail)
{
    struct disir_mold *invalid_mold;
    std::string json;

    invalid_mold = NULL;

    ASSERT_EQ (DPLUGIN_FATAL_ERROR, writer->marshal (invalid_mold, json));
}

TEST_F (MarshallMoldTest, DISABLED_tets)
{

}
