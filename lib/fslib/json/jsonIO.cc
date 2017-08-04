// JSON private
#include "json/dplugin_json.h"

// public
#include <disir/disir.h>

// standard
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdarg.h>

using namespace dio;

struct cmp_str
{
   bool operator()(char const *a, char const *b)
   {
     return std::strcmp(a, b) < 0;
   }
};

//! Used for unserialization of molds - maps string types
//! to disir value types
std::map<const char *, enum disir_value_type, cmp_str> attribute_disir_value_keys  = {
    {"BOOLEAN", DISIR_VALUE_TYPE_BOOLEAN},
    {"INTEGER", DISIR_VALUE_TYPE_INTEGER},
    {"ENUM"   , DISIR_VALUE_TYPE_ENUM},
    {"FLOAT"  , DISIR_VALUE_TYPE_FLOAT},
    {"STRING" , DISIR_VALUE_TYPE_STRING}};

//! Used for unserialization of molds - maps string types
//! to disir restriction types
std::map<const char *, enum disir_restriction_type, cmp_str> attribute_disir_restriction_keys  = {
    {"MINIMUM_ENTRIES", DISIR_RESTRICTION_INC_ENTRY_MIN},
    {"MAXIMUM_ENTRIES", DISIR_RESTRICTION_INC_ENTRY_MAX},
    {"ENUM"           , DISIR_RESTRICTION_EXC_VALUE_ENUM},
    {"RANGE"          , DISIR_RESTRICTION_EXC_VALUE_RANGE},
    {"NUMERIC"        , DISIR_RESTRICTION_EXC_VALUE_NUMERIC}};

//! Map holding string representation of json valuetypes
std::map<Json::ValueType, const char *> json_type_stringified  = {
    {Json::intValue    , "integer"},
    {Json::realValue   , "float"},
    {Json::booleanValue , "boolean"},
    {Json::stringValue , "string"},
    {Json::arrayValue  , "array"},
    {Json::nullValue   , "null"},
    {Json::objectValue , "object"}};

JsonIO::JsonIO (struct disir_instance *disir)
{
    m_disir = disir;
}

enum disir_status
assert_json_value_type (Json::Value& value, Json::ValueType type)
{
 return (value.type () == type) ? DISIR_STATUS_OK : DISIR_STATUS_WRONG_VALUE_TYPE;
}

enum Json::ValueType
disir_value_to_json_value (enum disir_value_type type)
{
    switch (type)
    {
    case DISIR_VALUE_TYPE_INTEGER:
        return Json::intValue;
    case DISIR_VALUE_TYPE_FLOAT:
        return Json::realValue;
    case DISIR_VALUE_TYPE_BOOLEAN:
        return Json::booleanValue;
    case DISIR_VALUE_TYPE_ENUM:
        return Json::stringValue;
    case DISIR_VALUE_TYPE_STRING:
        return Json::stringValue;
    default:
        return Json::nullValue;
    }
}

enum disir_status
set_value (struct disir_context *context, Json::Value& val)
{
    switch (val.type())
    {
    case Json::intValue:
    {
        if (dc_value_type (context) == DISIR_VALUE_TYPE_FLOAT)
        {
            return dc_set_value_float (context, (double)val.asDouble());
        }
        else
        {
            return dc_set_value_integer (context, val.asInt64());
        }
    }
    case Json::booleanValue:
        return dc_set_value_boolean (context, val.asBool());
    case Json::realValue:
        return dc_set_value_float (context, (double)val.asDouble());
    case Json::stringValue:
    {
        if (dc_value_type (context) == DISIR_VALUE_TYPE_ENUM)
        {
            return dc_set_value_enum (context,
                    val.asString().c_str(), val.asString().size());
        }
        else
        {
            return dc_set_value_string (context,
                    val.asString().c_str(), val.asString().size());
        }
    }
    case Json::arrayValue:
    case Json::uintValue:
    case Json::objectValue:
    case Json::nullValue:
    default:
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }
}

enum disir_status
add_value_default (struct disir_context *context, Json::Value& value,
                   struct disir_version *version)
{
    switch (value.type())
    {
    case Json::intValue:
    {
        if (dc_value_type (context) == DISIR_VALUE_TYPE_FLOAT)
        {
            return dc_add_default_float (context, (double)value.asDouble(), version);
        }
        else
        {
            return dc_add_default_integer (context, value.asInt64(), version);
        }
    }
    case Json::booleanValue:
        return dc_add_default_boolean (context, (uint8_t)value.asInt64(), version);
    case Json::realValue:
        return dc_add_default_float (context, (double)value.asDouble(), version);
    case Json::stringValue:
        return dc_add_default_string (context, value.asString().c_str(),
                                               value.asString().size(), version);
    case Json::arrayValue:
    case Json::uintValue:
    case Json::objectValue:
    case Json::nullValue:
    default:
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }
}

enum disir_value_type
attribute_key_to_disir_value (const char *type)
{
    try
    {
        return attribute_disir_value_keys.at (type);
    }
    catch (std::out_of_range e)
    {
        return DISIR_VALUE_TYPE_UNKNOWN;
    }
}

enum disir_restriction_type
attribute_key_to_disir_restriction (const char *type)
{
    try
    {
        return attribute_disir_restriction_keys.at (type);
    }
    catch (std::out_of_range e)
    {
        return DISIR_RESTRICTION_UNKNOWN;
    }
}

const char *
json_valuetype_stringify (Json::ValueType type)
{
    try
    {
        return json_type_stringified.at (type);
    }
    catch (std::out_of_range e)
    {
        return "unknown";
    }
}
