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

JsonIO::JsonIO (struct disir_instance *disir)
{
    m_disir = disir;
}

enum disir_status
JsonIO::read_json_from_file (const char *filepath, Json::Value& root)
{
    std::ifstream file (filepath);
    std::stringstream buffer;
    Json::Reader reader;

    if (file.is_open())
    {
        buffer << file.rdbuf();
    }
    else
    {
        disir_error_set (m_disir, "Could not open file on filepath: %s", strerror (errno));
        return DISIR_STATUS_FS_ERROR;
    }

    bool success = reader.parse (buffer, root);
    if (!success)
    {
        disir_error_set (m_disir, "Could not parse jsonfile: %s",
                                   reader.getFormattedErrorMessages ().c_str ());
        return DISIR_STATUS_FS_ERROR;
    }
    return DISIR_STATUS_OK;
}

void
JsonIO::append_disir_error (Json::OrderedValueIterator& object, const char *message, ...)
{
    va_list args;
    std::stringstream ss;
    std::string format;

    if ((*object).isNull () == false)
    {
        ss << object.name () << " on line: " << (*object).getOffsetStart () << " -> ";
        format = ss.str();
    }
    else
    {
        format = "";
    }

    va_start (args, message);

    add_error (format.c_str(), message,  args);

    va_end (args);
}

void
JsonIO::append_disir_error (Json::ValueIterator& object, const char *message, ...)
{
    va_list args;
    std::stringstream ss;
    std::string format;

    if ((*object).isNull () == false)
    {
        ss << object.name () << " on line: " << (*object).getOffsetStart () << " -> ";
        format = ss.str();
    }
    else
    {
        format = "";
    }

    va_start (args, message);

    add_error (format.c_str (), message,  args);

    va_end (args);
}

void
JsonIO::append_disir_error (const char *message, ...)
{
    va_list args;

    va_start (args, message);

    add_error (NULL, message, args);

    va_end (args);
}

void
JsonIO::add_error (const char *prefix, const char *message, va_list args)
{
    int bytes;
    char buf[500];

    bytes = 0;

    if (prefix)
    {
        bytes = sprintf (buf, "%s", prefix);
    }

    bytes = vsprintf (&buf[bytes], message, args);

    m_errors.push_back (buf);
}


void
JsonIO::populate_disir_with_errors ()
{
    std::string err;

    for (auto it = m_errors.begin (); it != m_errors.end (); ++it)
    {
        err += *it;
    }

    if (err.empty () == false)
    {
        disir_error_set (m_disir, err.c_str ());
    }
}

enum disir_status
assert_json_value_type (Json::Value& value, Json::ValueType type)
{
 return (value.type () == type) ? DISIR_STATUS_OK : DISIR_STATUS_WRONG_VALUE_TYPE;
}

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



