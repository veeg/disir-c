#include "test_json.h"


using namespace testing;

bool
JsonDioTestWrapper::GetJsonObject (Json::Value& root, std::string path)
{
    std::ifstream config (path);
    std::stringstream buffer;
    Json::Reader reader;

    if (config)
    {
        buffer << config.rdbuf();
    }
    else
    {
        return false;
    }

    bool success = reader.parse (buffer.str(), root);
    if (!success) {
        std::cerr << reader.getFormattedErrorMessages() << std::endl;
        log_test ("Could not parse jsonfile: %s",
                    reader.getFormattedErrorMessages().c_str());
        return false;
    }
    return success;
}

bool
JsonDioTestWrapper::assert_invalid_context (struct disir_mold *mold, const char *name,
                                            const char *context_type, const char *errormsg)
{
    enum disir_status status;
    struct disir_context *context;
    struct disir_collection *collection;
    const char *context_name;
    status = disir_mold_valid (mold, &collection);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    status = DISIR_STATUS_NOT_EXIST;

    while (dc_collection_next (collection, &context) != DISIR_STATUS_EXHAUSTED)
    {
        if (strcmp (dc_context_type_string (context), context_type) == 0 &&
            ((dc_context_error (context) == NULL && errormsg == NULL) ||
            (errormsg != NULL && strcmp (dc_context_error (context), errormsg) == 0)))
        {
            if (name != NULL)
            {
                dc_get_name (context, &context_name, NULL);
                if (strcmp (context_name, name) != 0)
                    continue;

                status = DISIR_STATUS_OK;
            }
            else
            {
                status = DISIR_STATUS_OK;
            }
            goto out;
        }
    }
    // FALL-THROUGH
out:
    dc_collection_finished (&collection);

    return (DISIR_STATUS_OK == status);
}

int
JsonDioTestWrapper::invalid_context_count (struct disir_mold *mold)
{
    enum disir_status status;
    struct disir_collection *collection;

    status = disir_mold_valid (mold, &collection);
    EXPECT_STATUS (DISIR_STATUS_INVALID_CONTEXT, status);

    auto invalid_contexts = dc_collection_size (collection);

    dc_collection_finished (&collection);

    return invalid_contexts;
}

bool
JsonDioTestWrapper::compare_json_objects (std::string a, std::string b)
{
    Json::Reader reader;
    Json::Value rootA;
    Json::Value rootB;

    reader.parse (a.c_str(), rootA);
    reader.parse (b.c_str(), rootB);

    return _compare_json_objects (rootA, rootB);
}

bool
JsonDioTestWrapper::GetJsonConfig (Json::Value& obj)
{
    return GetJsonObject (obj, m_configjsonPath);
}

bool
JsonDioTestWrapper::GetJsonMold (Json::Value& obj)
{
    return GetJsonObject (obj, m_moldjsonPath);
}

bool
JsonDioTestWrapper::formatJson (std::string& json, std::string unformatted)
{
    Json::Reader reader;
    Json::StyledWriter writer;
    Json::Value root;

    bool success = reader.parse (unformatted, root);
    if (!success) {
        return success;
    }
    json = writer.writeOrdered (root);

    return true;
}

std::string
JsonDioTestWrapper::getJsonString (Json::Value& obj)
{
    Json::StyledWriter writer;
    return writer.writeOrdered (obj);
}

bool
JsonDioTestWrapper::check_context_validity (struct disir_config *config,
        const char *context_name)
{
    struct disir_context *context_config;
    struct disir_context *queried_context;
    struct disir_collection *collection;

    context_config = dc_config_getcontext (config);

    status = dc_find_elements (context_config, context_name, &collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_collection_next (collection, &queried_context);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_context_valid (queried_context);

    // Cleanup
    dc_putcontext (&context_config);
    dc_putcontext (&queried_context);
    dc_collection_finished (&collection);

    if (status == DISIR_STATUS_INVALID_CONTEXT)
    {
        return true;
    }
    return false;
}

bool
JsonDioTestWrapper::check_context_validity (struct disir_context *context,
        const char *context_name)
{
    struct disir_context *queried_context;
    struct disir_collection *collection;

    status = dc_find_elements (context, context_name, &collection);
    EXPECT_STATUS (DISIR_STATUS_OK, status);
    status = dc_collection_next (collection, &queried_context);
    EXPECT_STATUS (DISIR_STATUS_OK, status);

    status = dc_context_valid (queried_context);

    // Cleanup
    dc_putcontext (&queried_context);
    dc_collection_finished (&collection);

    if (status == DISIR_STATUS_INVALID_CONTEXT)
    {
        return true;
    }
    return false;
}


std::string
JsonDioTestWrapper::getMoldJson ()
{
    Json::StyledWriter writer;
    Json::Value root;
    GetJsonMold (root);
    return writer.writeOrdered (root);
}

int
JsonDioTestWrapper::GenerateConfigFromJson (Json::Value&)
{
    return -1;
}

bool
JsonDioTestWrapper::_compare_json_objects (Json::Value& objOne,
        Json::Value& objTwo)
{
    if (!(objOne == objTwo))
    {
        std::cerr << "False" << std::endl;
        return false;
    }

    Json::OrderedValueIterator iter1 = objOne.beginOrdered ();
    Json::OrderedValueIterator iter2 = objTwo.beginOrdered ();
    std::string errString;
    Json::Value valueOne, valueTwo;

    for (; iter1 != objOne.endOrdered () && iter2 != objTwo.endOrdered (); ++iter1, ++iter2)
    {
        valueOne = *iter1;
        valueTwo = *iter2;
        if (iter1.name () != iter2.name ())
        {
            std::cerr << iter1.name() << " " <<  iter2.name() << std::endl;
            return false;
        }

        if (valueOne.isObject () && valueTwo.isObject ())
        {
            return _compare_json_objects (*iter1, *iter2);
        }
        else if (valueOne.isArray() && valueTwo.isArray ())
        {
            return _compare_json_objects (*iter1, *iter2);
        }
    }

    return true;
}

