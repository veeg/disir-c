// Local json
#include "json/json_mold_override.h"
#include "json/dplugin_json.h"

// public
#include <disir/disir.h>

// standard
#include <iostream>
#include <stdarg.h>
#include <cstring>
#include <set>

using namespace dio;

MoldOverride::MoldOverride ()
{
}

//! PUBLIC
enum disir_status
MoldOverride::parse_mold_override_entry (struct disir_instance *instance, std::istream& entry)
{
    enum disir_status status;
    Json::Reader reader;
    Json::Value root;

    bool success = reader.parse (entry, root);
    if (!success)
    {
        std::cout << reader.getFormattedErrorMessages() << std::endl;
        disir_error_set (instance, "mold override parse error: %s",
                                   reader.getFormattedErrorMessages().c_str());
        return DISIR_STATUS_FS_ERROR;
    }

    status = validate_override_entry (instance, root);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    return parse_mold_overrides (root);
}

enum disir_status
MoldOverride::validate_override (struct disir_instance *instance,
                                 Json::Value& current, std::string& name,
                                 std::set<std::string>& versions)
{
    auto version = current[ATTRIBUTE_KEY_VERSION];
    auto value = current[ATTRIBUTE_KEY_VALUE];

    if (version.isNull())
    {
        disir_error_set (instance,
                         "invalid mold override entry: (%s) missing version", name.c_str());
        return DISIR_STATUS_FS_ERROR;

    }
    if (version.isString() == false)
    {
        disir_error_set (instance,
                         "invalid mold override entry: (%s) version not string", name.c_str());
        return DISIR_STATUS_FS_ERROR;
    }
    if (value.isNull())
    {
        disir_error_set (instance,
                         "invalid mold override entry: (%s) missing value", name.c_str());
        return DISIR_STATUS_FS_ERROR;
    }
    if (versions.find (version.asString()) == versions.end())
    {
        disir_error_set (instance, "invalid mold override entry: (%s) "
                                   "version not accounted for in sync mapping",
                                   name.c_str());
        return DISIR_STATUS_FS_ERROR;
    }

    return DISIR_STATUS_OK;
}

enum disir_status
MoldOverride::validate_override_entry (struct disir_instance *instance, Json::Value& root)
{
    enum disir_status status;
    std::set<std::string> versions;

    // for an override entry to be valid the following invaraints
    // must be fulfilled:
    // It must have "sync" array and "override" object
    // All "sync" entries (objects) in turn must have "namespace" and "override" values
    auto sync_entries = root[ATTRIBUTE_KEY_SYNC];
    auto override_entries = root[ATTRIBUTE_KEY_OVERRIDE];

    //! We do not have an override entry
    if (sync_entries.isNull() && override_entries.isNull())
    {
        //! No error msg necessarry
        return DISIR_STATUS_NOT_EXIST;
    }

    // Sync entries is not of correct type
    if (sync_entries.isNull() == false && sync_entries.isArray() == false)
    {
        disir_error_set (instance, "invalid mold override entry: Sync entry is not of type array");
        return DISIR_STATUS_FS_ERROR;
    }

    if (override_entries.isNull() == false && override_entries.isObject() == false)
    {
        disir_error_set (instance,
                         "invalid Mold override entry: Override entry is not of type object");
        return DISIR_STATUS_FS_ERROR;
    }

    for (unsigned int i = 0; i < sync_entries.size(); i++)
    {
        auto sync_entry = sync_entries[i];
        auto namespace_version = sync_entry[ATTRIBUTE_KEY_NAMESPACE];
        auto override_version = sync_entry[ATTRIBUTE_KEY_OVERRIDE];

        if (override_version.isNull())
        {
            disir_error_set (instance,
                             "invalid sync entry: sync entry (%d) missing override version", i);
            return DISIR_STATUS_FS_ERROR;
        }
        if (namespace_version.isNull())
        {
            disir_error_set (instance,
                             "invalid sync entry: sync entry (%d) missing namespace version", i);
            return DISIR_STATUS_FS_ERROR;
        }
        if (override_version.isString() == false)
        {
            disir_error_set (instance,
                             "invalid sync entry: sync entry (%d) override version not string", i);
            return DISIR_STATUS_FS_ERROR;
        }
        if (namespace_version.isString() == false)
        {
            disir_error_set (instance,
                             "invalid sync entry: sync entry (%d) namespace version not string", i);
            return DISIR_STATUS_FS_ERROR;
        }
        versions.insert (override_version.asString());
    }

    // Loop through and assert that each override entry is valid and
    // that their versions are accounted for in the sync mapping
    for (auto iter = override_entries.begin(); iter != override_entries.end(); iter++)
    {
        Json::Value& current = *iter;
        auto name = iter.name();

        if (current.type() == Json::arrayValue)
        {
            for (unsigned int i = 0; i < current.size(); i++)
            {
                status = validate_override (instance, current[i], name, versions);
                if (status != DISIR_STATUS_OK)
                {
                    return status;
                }
            }
        }
        else
        {
            status = validate_override (instance, current, name, versions);
            if (status != DISIR_STATUS_OK)
            {
                return status;
            }
        }
    }

    return DISIR_STATUS_OK;
}

//! PRIVATE
enum disir_status
MoldOverride::string_to_disir_version (Json::Value& current, const char *attribute_key,
                                          struct disir_version *version)
{
    enum disir_status status;

    status = assert_json_value_type (current[attribute_key], Json::stringValue);
    if (status != DISIR_STATUS_OK)
    {
        return DISIR_STATUS_FS_ERROR;
    }

    status = dc_version_convert (current[attribute_key].asCString(), version);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    return status;
}

enum disir_status
MoldOverride::parse_override_sync_mapping (Json::Value& sync_entries, syncmap& mapping)
{
    enum disir_status status;
    struct disir_version version_override;
    struct disir_version version_namespace;

    status = DISIR_STATUS_OK;

    for (unsigned int i = 0; i < sync_entries.size(); i++)
    {
        auto namespace_version = sync_entries[i][ATTRIBUTE_KEY_NAMESPACE];
        auto override_version = sync_entries[i][ATTRIBUTE_KEY_OVERRIDE];

        status = dc_version_convert (namespace_version.asCString(), &version_namespace);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }

        status = dc_version_convert (override_version.asCString(), &version_override);
        if (status != DISIR_STATUS_OK)
        {
            return status;
        }

        mapping.insert (std::make_pair (version_override, version_namespace));
    }

    return status;
}

//! PRIVATE
enum disir_status
MoldOverride::populate_mold_entry_override (Json::Value& current, std::string name,
                                            syncmap& override_mapping)
{
    struct disir_version version;
    auto mold_entry_override = std::unique_ptr<struct mold_entry_override>
                                              (new struct mold_entry_override);

    mold_entry_override->mo_name = name;

    auto status = string_to_disir_version (current, ATTRIBUTE_KEY_VERSION, &version);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Check if we have matching entry in sync
    auto found = override_mapping.find (version);
    if (found != override_mapping.end())
    {
        mold_entry_override->mo_version = found->second;
    }
    else
    {
        // Crash hard - this case should have been covered by validate
        return DISIR_STATUS_INTERNAL_ERROR;
    }

    mold_entry_override->mo_value = current[ATTRIBUTE_KEY_VALUE];

    auto iter = m_override_entries.find (mold_entry_override->mo_name);
    if (iter != m_override_entries.end())
    {
        iter->second.insert (std::move (mold_entry_override));
    }
    else
    {
        m_override_entries[mold_entry_override->mo_name].insert (std::move (mold_entry_override));
    }

    return DISIR_STATUS_OK;
}

//! PRIVATE
enum disir_status
MoldOverride::parse_mold_overrides (Json::Value& root)
{
    enum disir_status status;
    syncmap override_mapping;

    status = parse_override_sync_mapping (root[ATTRIBUTE_KEY_SYNC], override_mapping);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    auto override_entries = root[ATTRIBUTE_KEY_OVERRIDE];

    for (auto iter = override_entries.beginOrdered();
              iter != override_entries.endOrdered(); iter++)
    {
        Json::Value& current = *iter;

        if (current.type() == Json::arrayValue)
        {
            for (unsigned int i = 0; i < current.size(); i++)
            {
                status = populate_mold_entry_override (current[i], iter.name(),
                                                       override_mapping);
                if (status != DISIR_STATUS_OK)
                {
                    return status;
                }
            }
        }
        else if (current.type() == Json::objectValue)
        {
            status = populate_mold_entry_override (current, iter.name(), override_mapping);
            if (status != DISIR_STATUS_OK)
            {
                return status;
            }
        }
    }

    return DISIR_STATUS_OK;
}

//! PRIVATE
enum disir_status
MoldOverride::retrieve_highest_default_version (struct disir_context *context,
                                                struct disir_version *highest_version)
{
   enum disir_status status;
   struct disir_collection *collection;
   struct disir_context *context_default = NULL;
   struct disir_version version;

   status = dc_get_default_contexts (context, &collection);
   if (status != DISIR_STATUS_OK)
   {
        return status;
   }

   highest_version->sv_minor = 0;
   highest_version->sv_major = 0;

   // Go through all defaults and determine which
   // one has the highest version
   while (dc_collection_next (collection, &context_default)
          != DISIR_STATUS_EXHAUSTED)
    {
        status = dc_get_introduced (context_default, &version);
        if (status != DISIR_STATUS_OK)
            goto out;

        auto eq = dc_version_compare (&version, highest_version);
        if (eq >= 0)
        {
            *highest_version = version;
        }
        dc_putcontext (&context_default);
    }
    // FALL-THROUGH
out:
   if (context_default)
       dc_putcontext (&context_default);

   dc_collection_finished (&collection);

   return status;
}

enum disir_status
MoldOverride::set_value_existing_default (struct disir_context *context_keyval,
                                          struct disir_version *version_target,
                                          Json::Value& value)
{
   enum disir_status status;
   struct disir_collection *collection;
   struct disir_context *context_default = NULL;
   struct disir_version version;

   status = dc_get_default_contexts (context_keyval, &collection);
   if (status != DISIR_STATUS_OK)
   {
        return status;
   }

   // Go through all defaults and find a default with matching version
   while (dc_collection_next (collection, &context_default)
          != DISIR_STATUS_EXHAUSTED)
    {
        status = dc_get_introduced (context_default, &version);
        if (status != DISIR_STATUS_OK)
            goto out;

        if (dc_version_compare (&version, version_target) == 0)
        {
            status = set_value (value, context_default);
            if (status != DISIR_STATUS_OK)
                goto out;
        }
        dc_putcontext (&context_default);
    }
    // FALL-THROUGH
out:
   if (context_default)
       dc_putcontext (&context_default);

   dc_collection_finished (&collection);

   return status;
}

enum disir_status
MoldOverride::apply_override (struct disir_context *context_mold, struct mold_entry_override *entry,
                              struct disir_version *mold_version)
{
    enum disir_status status;
    struct disir_version highest_default;
    struct disir_context *context_keyval = NULL;
    int eq;

    status = dc_query_resolve_context (context_mold, entry->mo_name.c_str(), &context_keyval);
    if (status != DISIR_STATUS_OK && status != DISIR_STATUS_NOT_EXIST)
    {
        return status;
    }
    else if (status == DISIR_STATUS_NOT_EXIST)
    {
        dc_fatal_error (context_mold, "override mold keyval '%s' does not exist in namespace mold",
                                      entry->mo_name.c_str());
        return DISIR_STATUS_INVALID_CONTEXT;
    }

    status = retrieve_highest_default_version (context_keyval, &highest_default);
    if (status != DISIR_STATUS_OK)
        goto out;

    // Override version cannot be higher than the mold version
    if (dc_version_compare (&entry->mo_version, mold_version) > 0)
    {
        char mbuf[100];
        char kbuf[100];
        dc_fatal_error (context_mold,  "keyval '%s' override version (%s) is higher than "
                                        "namespace mold version (%s)",
                                        entry->mo_name.c_str(),
                                        dc_version_string (kbuf, 100, &entry->mo_version),
                                        dc_version_string (mbuf, 100, mold_version));
        status = DISIR_STATUS_INVALID_CONTEXT;
        goto out;
    }

    // mo_version contains which version to override (equal or lower)
    eq = dc_version_compare (&entry->mo_version, &highest_default);
    if (eq > 0)
    {
        status = add_value_default (context_keyval, entry->mo_value, &entry->mo_version);
    }
    else if (eq == 0)
    {
        status = set_value_existing_default (context_keyval,
                                             &entry->mo_version, entry->mo_value);
    }
    if (status == DISIR_STATUS_WRONG_VALUE_TYPE)
    {
        dc_fatal_error (context_mold,  "override type mismatch on keyval '%s'. Should be %s",
                                        entry->mo_name.c_str(),
                                        dc_value_type_string (context_keyval));
        status = DISIR_STATUS_INVALID_CONTEXT;
    }
    // FALL-THROUGH
out:
    if (context_keyval)
        dc_putcontext (&context_keyval);

    return status;
}

//! PUBLIC
enum disir_status
MoldOverride::apply_overrides (struct disir_context *context_mold)
{
    enum disir_status status;
    struct disir_version mold_version;

    status = dc_get_introduced (context_mold, &mold_version);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    for (const auto& kv : m_override_entries)
    {
        for (const auto& entry : kv.second)
        {
            status = apply_override (context_mold, entry.get (), &mold_version);
            if (status != DISIR_STATUS_OK)
            {
                return status;
            }
        }
    }

    return status;
}

