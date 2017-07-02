#ifndef _TEST_HELPER_H
#define _TEST_HELPER_H

#include "test_helper.h"

#include <json/json.h>
#include <fstream>

#define ASSERT_JSON_STREQ(a,b)                  \
{                                               \
    if (strcmp(a,b) != 0)                       \
    {                                           \
        FAIL () << "Expected string: \n"        \
        << a                                    \
        << "actual string \n"                   \
        << b                                    \
        << "\n";                                \
    }                                           \
}

#define ASSERT_INVALID_CONTEXT_EXIST(mold,name,disir_context_type,error)         \
{                                                                                \
    if (assert_invalid_context (mold,name, disir_context_type, error) == false)  \
    {                                                                            \
        FAIL () << "No invalid context of type "                                 \
                << disir_context_type                                            \
                << " with error "                                                \
                << error << "\n";                                                \
    }                                                                            \
}

#define ASSERT_INVALID_CONTEXT_COUNT(mold,count)                            \
{                                                                           \
    if (invalid_context_count (mold) != count)                              \
    {                                                                       \
        FAIL () << "Wrong invalid context count\n"                          \
                << "Asserted count "                                        \
                << count                                                    \
                << ", got "                                                 \
                << invalid_context_count (mold);                            \
    }                                                                       \
}

namespace testing
{
    class JsonDioTestWrapper : public testing::DisirTestTestPlugin
    {
        protected:
            const std::string m_configjsonPath
                = CMAKE_PROJECT_SOURCE_DIR "/test/plugins/json/json/reference_config.json";

            const std::string m_moldjsonPath
                = CMAKE_PROJECT_SOURCE_DIR "/test/plugins/json/json/reference_mold.json";

            const std::string m_jsonPath = CMAKE_PROJECT_SOURCE_DIR "/test/plugins/json/json/";

            bool _compare_json_objects (Json::Value& objOne, Json::Value& objTwo);

            // Variables
            enum disir_status status = DISIR_STATUS_OK;

        public:
            //! \breif compares two json strings
            bool compare_json_objects (std::string a, std::string b);

            bool GetJsonObject (Json::Value& root, std::string path);
            bool GetJsonConfig (Json::Value& obj);

            bool
            assert_invalid_context (struct disir_mold *mold, const char *name,
                                    const char *context_type, const char *errormsg);

            int
            invalid_context_count (struct disir_mold *mold);

            std::string getMoldJson ();

            //! \brief checks whether a context with context_name is valid or not
            bool check_context_validity (struct disir_config *config, const char *context_name);

            //! \brief checks whether a context with context_name is valid or not
            bool check_context_validity (struct disir_context *context, const char *context_name);

            bool GetJsonMold (Json::Value& obj);

            bool formatJson (std::string& json,  std::string unformatted);

            std::string getJsonString (Json::Value& obj);

            int GenerateConfigFromJson (Json::Value& obj);
    };
}
#endif
