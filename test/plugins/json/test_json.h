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

namespace testing
{
    class JsonDioTestWrapper : public testing::Test
    {
        protected:
            const std::string m_configjsonPath
                = CMAKE_PROJECT_SOURCE_DIR "/test/json/reference_config.json";

            const std::string m_moldjsonPath
                = CMAKE_PROJECT_SOURCE_DIR "/test/json/reference_mold.json";

            const std::string m_jsonPath = CMAKE_PROJECT_SOURCE_DIR "/test/json/";

            static void SetUpTestCase ();

            bool _compare_json_objects (Json::Value& objOne, Json::Value& objTwo);

           static void TearDownTestCase ();

            void SetUpDisir ();
            void TearDownDisir ();

            // Variables
            enum disir_status status;
            struct disir_instance *disir;

        public:
            //! \breif compares two json strings
            bool compare_json_objects (std::string a, std::string b);

            bool GetJsonObject (Json::Value& root, std::string path);
            bool GetJsonConfig (Json::Value& obj);

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
