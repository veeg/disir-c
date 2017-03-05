#include <disir/disir.h>
#include <json/json.h>

enum disir_status
set_value (Json::Value& val, struct disir_context *context)
{
    switch (val.type())
    {
        case Json::intValue:
            return dc_set_value_integer (context, val.asInt64());
        case Json::booleanValue:
            return dc_set_value_boolean (context, val.asBool());
        case Json::realValue:
            return dc_set_value_float (context, (double)val.asDouble());
        case Json::stringValue:
            return dc_set_value_string (context,
                    val.asString().c_str(), val.asString().size());
        case Json::arrayValue:
            return DISIR_STATUS_WRONG_VALUE_TYPE;
        case Json::uintValue:
            return DISIR_STATUS_WRONG_VALUE_TYPE;
        case Json::objectValue:
            return DISIR_STATUS_WRONG_VALUE_TYPE;
        case Json::nullValue:
            return DISIR_STATUS_WRONG_VALUE_TYPE;
        default:
            return DISIR_STATUS_WRONG_VALUE_TYPE;
    }
}

enum disir_value_type
string_to_type (std::string type)
{
    if (type == "BOOLEAN")
    {
        return DISIR_VALUE_TYPE_BOOLEAN;
    }
    else if (type == "STRING")
    {
        return DISIR_VALUE_TYPE_STRING;
    }
    else if (type == "FLOAT")
    {
        return DISIR_VALUE_TYPE_FLOAT;
    }
    else if (type == "INTEGER")
    {
        return DISIR_VALUE_TYPE_INTEGER;
    }
    else
    {
        return DISIR_VALUE_TYPE_UNKNOWN;
    }
}
