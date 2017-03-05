#include "dplugin_json.h"
#include <iostream>
#include <cstring>
#include <fstream>
#include <disir/disir.h>
#include <stdarg.h>

using namespace dio;

JsonIO::JsonIO (struct disir_instance *disir)
{
    m_disir = disir;
}

enum dplugin_status
JsonIO::read_config (const char *filepath, Json::Value& root)
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
        return DPLUGIN_IO_ERROR;
    }

    bool success = reader.parse (buffer, root);
    if (!success)
    {
        disir_error_set (m_disir, "Could not parse jsonfile: %s",
                                   reader.getFormattedErrorMessages ().c_str ());
        return DPLUGIN_PARSE_ERROR;
    }
    return DPLUGIN_STATUS_OK;
}


enum dplugin_status
object_members_check (Json::Value& object, ...)
{
    const char *variadic_type;
    va_list ap;
    va_start (ap, object);

    try {
        for (variadic_type = va_arg (ap, const char*);
             variadic_type != NULL;
             variadic_type = va_arg (ap, const char*))
        {
            // throws an exception if it does not exist
            // or if Json::Value is not of type object nor
            // array.
            if(object[variadic_type].isNull())
            {
                return DPLUGIN_FATAL_ERROR;
            }
        }
        va_end (ap);
    }
    catch (std::exception& e)
    {
        return DPLUGIN_FATAL_ERROR;
    }
    return DPLUGIN_STATUS_OK;
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
        bytes = sprintf (buf, prefix);
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

