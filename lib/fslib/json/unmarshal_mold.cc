// JSON private
#include "json/input.h"

// public
#include <disir/disir.h>

// standard
#include <iostream>
#include <stdarg.h>
#include <cstring>

using namespace dio;

//! Contructor
MoldReader::MoldReader (struct disir_instance *disir) : JsonIO (disir) {}

//! PUBLIC
enum disir_status
MoldReader::unmarshal (const char *filepath, struct disir_mold **mold)
{
    enum disir_status status;

    status = read_json_from_file (filepath, m_moldRoot);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    return construct_mold (mold);
}

//! PUBLIC
enum disir_status
MoldReader::unmarshal (std::istream& stream, struct disir_mold **mold)
{
    Json::Reader reader;

    bool success = reader.parse (stream, m_moldRoot);
    if (!success)
    {
        disir_error_set (m_disir, "Parse error: %s",
                                  reader.getFormattedErrorMessages().c_str());
        return DISIR_STATUS_FS_ERROR;
    }

    return construct_mold (mold);
}

enum disir_status
MoldReader::construct_mold (struct disir_mold **mold)
{
    struct disir_context *context_mold = NULL;
    enum disir_status status;

    if (m_moldRoot[ATTRIBUTE_KEY_MOLD].isNull ())
    {
        disir_error_set (m_disir, "No mold present");
        return DISIR_STATUS_FS_ERROR;
    }

    status = dc_mold_begin (&context_mold);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    if (mold_has_documentation (m_moldRoot))
    {
        auto doc = m_moldRoot[ATTRIBUTE_KEY_DOCUMENTATION].asString ();
        status = dc_add_documentation (context_mold, doc.c_str (), doc.size ());
        if (status != DISIR_STATUS_OK)
        {

            return status;
        }
    }

    if (m_moldRoot[ATTRIBUTE_KEY_MOLD].isNull ())
    {
        dc_fatal_error (context_mold, "No Mold present");
        goto finalize;
    }

    status = _unmarshal_mold (context_mold, m_moldRoot[ATTRIBUTE_KEY_MOLD]);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

finalize:
    status = dc_mold_finalize (&context_mold, mold);
    if (status != DISIR_STATUS_OK)
    {
        goto error;
    }

    return status;
error:
    if (context_mold)
    {
        // delete
        dc_destroy (&context_mold);
    }

    return status;
}

bool
MoldReader::mold_has_documentation (Json::Value& mold_root)
{
    return mold_root[ATTRIBUTE_KEY_DOCUMENTATION].isNull () == false &&
           mold_root[ATTRIBUTE_KEY_DOCUMENTATION].isString ();
}

bool
MoldReader::value_is_section (Json::Value& val)
{
    return val.isObject () && val [ATTRIBUTE_KEY_ELEMENTS].isNull () == false &&
                              val[ATTRIBUTE_KEY_ELEMENTS].isObject ();
}

bool
MoldReader::value_is_keyval (Json::Value& val)
{
    return val.isObject () &&  val [ATTRIBUTE_KEY_DEFAULTS].isNull () == false &&
                               val[ATTRIBUTE_KEY_DEFAULTS].isArray ();
}

//! PRIVATE
enum disir_status
MoldReader::_unmarshal_mold (struct disir_context *parent_context, Json::Value& parent)
{
    Json::Value child_node;
    struct disir_context *child_context = NULL;
    enum disir_status status;

    status = DISIR_STATUS_OK;

    Json::OrderedValueIterator iter = parent.beginOrdered ();

    for (; iter != parent.endOrdered(); ++iter)
    {
        child_node = *iter;
        // if node is of tpe object and contains the keyval "elements", we interpret it
        // as a mold section
        if (value_is_section (child_node))
        {
            status = dc_begin (parent_context, DISIR_CONTEXT_SECTION, &child_context);
            if (status != DISIR_STATUS_OK)
            {
                return status;
            }

            status = set_context_attributes (child_context, iter, DISIR_CONTEXT_SECTION);

            if (status != DISIR_STATUS_OK)
            {
                return status;
            }

            status = unmarshal_restrictions (child_context, iter);
            if (status != DISIR_STATUS_OK
                && status != DISIR_STATUS_INVALID_CONTEXT)
            {
                return status;
            }

            status = _unmarshal_mold (child_context, (*iter)[ATTRIBUTE_KEY_ELEMENTS]);
            if (status != DISIR_STATUS_OK
                && status != DISIR_STATUS_INVALID_CONTEXT)
            {
                return status;
            }
        }
        // if node is of type object and contains the keyval "default" we interpret it as
        // a mold keyval
        else if (value_is_keyval (child_node))
        {
            status = dc_begin (parent_context, DISIR_CONTEXT_KEYVAL, &child_context);
            if (status != DISIR_STATUS_OK)
            {
                return status;
            }

            status = set_context_attributes (child_context, iter, DISIR_CONTEXT_KEYVAL);
            if (status != DISIR_STATUS_OK)
            {
                return status;
            }

            if (status != DISIR_STATUS_OK
                && status != DISIR_STATUS_INVALID_CONTEXT)
            {
                return status;
            }

            status = unmarshal_defaults (child_context, *iter);
            if (status != DISIR_STATUS_OK
                && status != DISIR_STATUS_INVALID_CONTEXT)
            {
                return status;
            }

            status = unmarshal_restrictions (child_context, iter);
            if (status != DISIR_STATUS_OK
                && status != DISIR_STATUS_INVALID_CONTEXT)
            {
                return status;
            }
        }
        else
        {
            dc_fatal_error (parent_context,
                            "Could not resolve whether object is of type keyval or section");
            return DISIR_STATUS_OK;
        }

        status = dc_finalize (&child_context);
        if (status != DISIR_STATUS_OK
            && status != DISIR_STATUS_INVALID_CONTEXT)
        {

            return status;
        }
    }

    return status;
}

enum disir_status
MoldReader::set_context_attributes (struct disir_context *context,
                                    Json::OrderedValueIterator& parent,
                                    enum disir_context_type type)
{
    enum disir_status status;
    Json::Value current;

    current = *parent;

    auto name = parent.name();

    status = dc_set_name (context, name.c_str(), name.size ());
    if (status != DISIR_STATUS_OK)
    {
        dc_fatal_error (context, "could not set name");
        return status;
    }

    if (type == DISIR_CONTEXT_KEYVAL &&
        current[ATTRIBUTE_KEY_TYPE].isNull ())
    {
        dc_fatal_error (context, "Keyval type is not present");
        return DISIR_STATUS_INVALID_CONTEXT;
    }

    // Only set type if it's not a section
    if(current[ATTRIBUTE_KEY_TYPE].isNull () == false)
    {
        status = assert_json_value_type (current[ATTRIBUTE_KEY_TYPE], Json::stringValue);
        if (status != DISIR_STATUS_OK)
        {
            dc_fatal_error (context, "Keyval type is not of type string");
            return DISIR_STATUS_OK;
        }

        auto value_type = attribute_key_to_disir_value (current[ATTRIBUTE_KEY_TYPE].asCString ());
        if (value_type == DISIR_VALUE_TYPE_UNKNOWN)
        {
            dc_fatal_error (context, "Undefined value");
            return DISIR_STATUS_OK;
        }

        status = dc_set_value_type (context, value_type);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
    }
    status = unmarshal_deprecated (context, current);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    auto doc = current[ATTRIBUTE_KEY_DOCUMENTATION];
    if (doc.isNull ())
    {
        // No documentation on this context exists
        return DISIR_STATUS_OK;
    }

    status = dc_add_documentation (context, doc.asCString (), strlen (doc.asCString ()));
    if (status != DISIR_STATUS_OK)
    {
        disir_log_user (m_disir, "could not add documentation: %s",
                                 disir_status_string (status));
        return status;
    }

    return DISIR_STATUS_OK;
}

enum disir_status
MoldReader::unmarshal_restriction (struct disir_context *context, Json::Value& current)
{
    enum disir_value_type value_type;
    enum disir_status status;
    struct disir_context *restriction;

    if (current[ATTRIBUTE_KEY_TYPE].isNull ())
    {
        dc_fatal_error (context, "No restriction type");
        return DISIR_STATUS_OK;
    }

    status = assert_json_value_type (current[ATTRIBUTE_KEY_TYPE], Json::stringValue);
    if (status != DISIR_STATUS_OK)
    {
        dc_fatal_error (context, "Restriction type should be of type string");
        return DISIR_STATUS_OK;
    }

    auto restriction_type = attribute_key_to_disir_restriction (
                                                        current[ATTRIBUTE_KEY_TYPE].asCString ());
    if (restriction_type == DISIR_RESTRICTION_UNKNOWN)
    {
        dc_fatal_error (context, "Unknown restriction type");
        return DISIR_STATUS_OK;
    }

    // XXX: check return status
    dc_get_value_type (context, &value_type);

    status = dc_begin (context, DISIR_CONTEXT_RESTRICTION, &restriction);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    status = unmarshal_introduced (restriction, current);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    status = unmarshal_deprecated (restriction, current);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    status = dc_set_restriction_type (restriction, restriction_type);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    status = set_restriction_value (context, current);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }
    return status;
}

enum disir_status
MoldReader::set_restriction_value (struct disir_context *context, Json::Value& current)
{
    enum disir_status status;
    Json::Value value;
    enum disir_restriction_type restriction_type;

    status = dc_get_restriction_type (context, &restriction_type);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Checking if the correct key is
    // present according to restriction type
    switch (restriction_type)
    {
    case DISIR_RESTRICTION_INC_ENTRY_MIN:
    case DISIR_RESTRICTION_INC_ENTRY_MAX:
    case DISIR_RESTRICTION_EXC_VALUE_ENUM:
    case DISIR_RESTRICTION_EXC_VALUE_NUMERIC:
    {
        if (current[ATTRIBUTE_KEY_VALUE].isNull ())
        {
            dc_fatal_error (context, "No value present");
            return DISIR_STATUS_OK;
        }
        value = current[ATTRIBUTE_KEY_VALUE];
        break;
    }
    case DISIR_RESTRICTION_EXC_VALUE_RANGE:
    {
        if (current[ATTRIBUTE_KEY_VALUE_MIN].isNull ()
            || current[ATTRIBUTE_KEY_VALUE_MAX].isNull ())
        {
            dc_fatal_error (context, "No value present");
            return DISIR_STATUS_OK;
        }
        break;
    }
    case DISIR_RESTRICTION_UNKNOWN:
        break;
    }

    switch (restriction_type)
    {
    case DISIR_RESTRICTION_INC_ENTRY_MIN:
    {
        status = assert_json_value_type (value, Json::intValue);
        if (status != DISIR_STATUS_OK)
        {
            dc_fatal_error (context, "Wrong value type");
            return DISIR_STATUS_OK;
        }

        status = dc_restriction_set_numeric (context, value.asInt64 ());
        if (status != DISIR_STATUS_OK)
        {

            return status;
        }
        break;
    }
    case DISIR_RESTRICTION_INC_ENTRY_MAX:
    {
        status = assert_json_value_type (value, Json::intValue);
        if (status != DISIR_STATUS_OK)
        {
            dc_fatal_error (context, "Wrong value type");
            return DISIR_STATUS_OK;
        }

        status = dc_restriction_set_numeric (context, value.asInt64 ());
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
        break;
    }
    case DISIR_RESTRICTION_EXC_VALUE_NUMERIC:
    {
        if (assert_json_value_type (value, Json::intValue) != DISIR_STATUS_OK
            || assert_json_value_type (value, Json::realValue) != DISIR_STATUS_OK)
        if (status != DISIR_STATUS_OK)
        {
            dc_fatal_error (context, "Wrong value type");
            return DISIR_STATUS_OK;
        }

        status = dc_restriction_set_numeric (context, value.asDouble ());
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
        break;
    }
    case DISIR_RESTRICTION_EXC_VALUE_RANGE:
    {
        if (assert_json_value_type (current[ATTRIBUTE_KEY_VALUE_MIN], Json::intValue)
            != DISIR_STATUS_OK
            || assert_json_value_type (current[ATTRIBUTE_KEY_VALUE_MAX], Json::intValue)
               != DISIR_STATUS_OK
            || assert_json_value_type (current[ATTRIBUTE_KEY_VALUE_MIN], Json::realValue)
               != DISIR_STATUS_OK
            || assert_json_value_type (current[ATTRIBUTE_KEY_VALUE_MAX], Json::realValue)
               != DISIR_STATUS_OK)
        if (status != DISIR_STATUS_OK)
        {
            dc_fatal_error (context, "Wrong value type");
            return DISIR_STATUS_OK;
        }

        auto min = current[ATTRIBUTE_KEY_VALUE_MIN].asDouble ();
        auto max = current[ATTRIBUTE_KEY_VALUE_MAX].asDouble ();

        status = dc_restriction_set_range (context, min, max);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
        break;
    }
    case DISIR_RESTRICTION_EXC_VALUE_ENUM:
    {
        status = assert_json_value_type (value, Json::stringValue);
        if (status != DISIR_STATUS_OK)
        {
            dc_fatal_error (context, "Wrong value type");
            return DISIR_STATUS_OK;
        }

         status = dc_restriction_set_string (context, value.asCString ());
         if (status != DISIR_STATUS_OK)
         {
            return status;
         }
         break;
    }
    case DISIR_RESTRICTION_UNKNOWN:
    default:
        break;
    }
    return status;
}

enum disir_status
MoldReader::unmarshal_restrictions (struct disir_context *context,
                                    Json::OrderedValueIterator& it)
{
    uint32_t i;
    enum disir_status status;

    if ((*it)[ATTRIBUTE_KEY_RESTRICTIONS].isNull ())
    {
        // No restrictions present
        return DISIR_STATUS_OK;
    }

    auto restrictions = (*it)[ATTRIBUTE_KEY_RESTRICTIONS];

    status = assert_json_value_type (restrictions, Json::arrayValue);
    if (status != DISIR_STATUS_OK)
    {

        dc_fatal_error (context, "Restrictions is not of type array");
        return DISIR_STATUS_INVALID_CONTEXT;
    }

    for (i = 0; i < restrictions.size (); i++)
    {
        status = unmarshal_restriction (context, restrictions[i]);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }
    }
    return status;
}

enum disir_status
MoldReader::unmarshal_defaults (struct disir_context *context_keyval, Json::Value& current)
{
    struct disir_context *context_default = NULL;
    enum disir_status status;
    enum disir_status error;
    Json::Value defaults;
    Json::Value def;

    error = DISIR_STATUS_OK;
    status = DISIR_STATUS_OK;

    if (current[ATTRIBUTE_KEY_DEFAULTS].isNull ())
    {
        dc_fatal_error (context_keyval, "Missing default entries");
        return DISIR_STATUS_OK;
    }

    status = assert_json_value_type (current[ATTRIBUTE_KEY_DEFAULTS], Json::arrayValue);
    if (status != DISIR_STATUS_OK)
    {
        dc_fatal_error (context_keyval, "Defaults should be of type array");
        return DISIR_STATUS_OK;
    }

    defaults = current[ATTRIBUTE_KEY_DEFAULTS];

    // If there are no values in the array
    if (defaults.size () == 0)
    {
        dc_fatal_error (context_keyval, "Missing default entries");
        return status;
    }

    for (unsigned int i = 0; i < defaults.size (); i++)
    {
        def = defaults[i];
        status = dc_begin (context_keyval, DISIR_CONTEXT_DEFAULT, &context_default);
        if (status != DISIR_STATUS_OK)
        {
            disir_log_user (m_disir, "dc_begin resulted in error: %s",
                                     disir_status_string (status));
            return status;
        }

        if (def[ATTRIBUTE_KEY_VALUE].isNull ())
        {
            dc_fatal_error (context_default, "Default value is not present");
            error = DISIR_STATUS_NOT_EXIST;
        }

        // If value is not present
        // error is set and we finalize context
        // to notify about the error
        if (error == DISIR_STATUS_OK)
        {
            status = set_value (def[ATTRIBUTE_KEY_VALUE], context_default);
            if (status != DISIR_STATUS_OK &&
                status != DISIR_STATUS_INVALID_CONTEXT)
            {
                disir_log_user (m_disir, "Unable to set value on context: %s",
                                       disir_status_string (status));
                return status;
            }

            status = unmarshal_introduced (context_default, def);
            if (status != DISIR_STATUS_OK)
            {
                return status;
            }

            status = unmarshal_deprecated (context_default, def);
            if (status != DISIR_STATUS_OK)
            {

                return status;
            }
        }

        status = dc_finalize (&context_default);
        if (status != DISIR_STATUS_OK &&
            status != DISIR_STATUS_INVALID_CONTEXT)
        {
                std::cerr << disir_status_string (status) << std::endl;

             disir_error_set (m_disir, "could not finalize defaul: %s",
                                        disir_status_string (status));
             goto error;
        }
    }

    return DISIR_STATUS_OK;
error:
    if (context_default)
    {
        dc_destroy (&context_default);
    }
    return status;
}

enum disir_status
MoldReader::unmarshal_introduced (struct disir_context *context, Json::Value& current)
{
    enum disir_status status;
    struct semantic_version intro;

    if (current[ATTRIBUTE_KEY_INTRODUCED].isNull () == true)
    {
        return DISIR_STATUS_NOT_EXIST;
    }

    status = assert_json_value_type (current[ATTRIBUTE_KEY_INTRODUCED], Json::stringValue);
    if (status != DISIR_STATUS_OK)
    {
        dc_fatal_error (context, "Wrong type for introduced");
        return DISIR_STATUS_OK;
    }

    status = dc_semantic_version_convert (current.asCString(), &intro);
    if (status != DISIR_STATUS_OK)
    {
        dc_fatal_error (context, "Semamtic version introduced is not formatted correctly");
        return status;
    }

    return DISIR_STATUS_OK;

    status = dc_add_introduced (context, &intro);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }
}

enum disir_status
MoldReader::unmarshal_deprecated (struct disir_context *context, Json::Value& current)
{
    enum disir_status status;
    struct semantic_version semver;

    if (current[ATTRIBUTE_KEY_DEPRECATED].isNull ())
    {
        return DISIR_STATUS_OK;
    }

    if (current[ATTRIBUTE_KEY_DEPRECATED].type () != Json::stringValue)
    {
        dc_fatal_error (context, "Semantic version depricated is not string");
        return DISIR_STATUS_WRONG_VALUE_TYPE;
    }

    status = dc_semantic_version_convert (current[ATTRIBUTE_KEY_DEPRECATED].asCString (), &semver);
    if (status != DISIR_STATUS_OK)
    {
        dc_fatal_error (context, "Semantic version deprecated is not formated correctly");
        return status;
    }

    status = dc_add_deprecated (context, &semver);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }
    return status;
}
