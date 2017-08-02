
//!
//! This file implements the entire test catalog, by implementing all functions
//! required for a disir plugin that may serve configs generated from mold, and molds
//! defined programmatically. This is part of the extended disir library so that
//! it is easy to iterate and test new plugins.
//!

#include <map>
#include <utility>
#include <string.h>
#include <limits.h>

#include <disir/disir.h>
#include <disir/util.h>
#include <disir/plugin.h>
#include <disir/test.h>
#include <disir/fslib/util.h>

#include "mqueue.h"

// Include programmatic implementation of several molds
#include "basic_keyval.cc"
#include "basic_section.cc"
#include "json_test_mold.cc"
#include "restriction_keyval_numeric_types.cc"
#include "restriction_entries.cc"
#include "restriction_config_parent_keyval_min_entry.cc"
#include "restriction_config_parent_keyval_max_entry.cc"
#include "restriction_config_parent_section_max_entry.cc"
#include "restriction_section_parent_keyval_max_entry.cc"
#include "basic_version_difference.cc"
#include "complex_section.cc"
#include "config_query_permutations.cc"
#include "multiple_defaults.cc"

typedef enum disir_status (*output_mold)(struct disir_mold **);

//! Internal static map to store test molds by id
static std::map<std::string, output_mold> molds = {
    std::make_pair ("basic_keyval", basic_keyval),
    std::make_pair ("basic_section", basic_section),
    std::make_pair ("json_test_mold", json_test_mold),
    std::make_pair ("restriction_keyval_numeric_types", restriction_keyval_numeric_types),
    std::make_pair ("restriction_entries", restriction_entries),
    std::make_pair ("restriction_config_parent_keyval_min_entry",
                    restriction_config_parent_keyval_min_entry),
    std::make_pair ("restriction_config_parent_keyval_max_entry",
                    restriction_config_parent_keyval_max_entry),
    std::make_pair ("restriction_config_parent_section_max_entry",
                    restriction_config_parent_section_max_entry),
    std::make_pair ("restriction_section_parent_keyval_max_entry",
                    restriction_section_parent_keyval_max_entry),
    std::make_pair ("basic_version_difference", basic_version_difference),
    std::make_pair ("complex_section", complex_section),
    std::make_pair ("config_query_permutations", config_query_permutations),
    std::make_pair ("nested/basic_keyval", basic_keyval),
    std::make_pair ("nested/", basic_keyval),
    std::make_pair ("super/", basic_keyval),
    std::make_pair ("super/nested/", basic_keyval),
    std::make_pair ("super/nested/basic_keyval", basic_keyval),
    std::make_pair ("multiple_defaults", multiple_defaults),
};


enum disir_status
dio_test_config_read (struct disir_instance *instance,
                      struct disir_register_plugin *plugin, const char *entry_id,
                      struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    output_mold func_mold;
    char namespace_entry[PATH_MAX];

    (void) &instance;
    (void) &plugin;

    func_mold = molds[std::string(entry_id)];
    if (func_mold == NULL)
    {
        if (fslib_namespace_entry (entry_id, namespace_entry) == NULL)
            return DISIR_STATUS_INVALID_ARGUMENT;

        func_mold = molds[namespace_entry];
        if (func_mold == NULL)
            return DISIR_STATUS_INVALID_ARGUMENT;
    }

    status = func_mold (&mold);
    if (status != DISIR_STATUS_OK)
        return status;

    status = disir_generate_config_from_mold (mold, NULL, config);
    // We are finished with our allocated mold
    disir_mold_finished (&mold);
    return status;
}

enum disir_status
dio_test_config_entries (struct disir_instance *instance,
                         struct disir_register_plugin *plugin, struct disir_entry **entries)
{
    return dio_test_mold_entries (instance, plugin, entries);
}

enum disir_status
dio_test_config_query (struct disir_instance *instance, struct disir_register_plugin *plugin,
                       const char *entry_id, struct disir_entry **entry)
{
    (void) &instance;
    (void) &plugin;
    (void) &entry;

    if (molds[entry_id] == NULL)
        return DISIR_STATUS_NOT_EXIST;
    else
        return DISIR_STATUS_EXISTS;
}

enum disir_status
dio_test_mold_read (struct disir_instance *instance, struct disir_register_plugin *plugin,
                    const char *entry_id, struct disir_mold **mold)
{
    enum disir_status status;
    output_mold func_mold;
    char namespace_entry[PATH_MAX];

    (void) &instance;
    (void) &plugin;
    (void) &entry_id;

    func_mold = molds[entry_id];
    if (func_mold == NULL)
    {

        if (fslib_namespace_entry (entry_id, namespace_entry) == NULL)
            return DISIR_STATUS_INVALID_ARGUMENT;

        func_mold = molds[namespace_entry];
        if (func_mold == NULL)
        {
            return DISIR_STATUS_INVALID_ARGUMENT;
        }
    }

    status = func_mold (mold);
    if (status != DISIR_STATUS_OK)
        return status;

    return DISIR_STATUS_OK;;
}

enum disir_status
dio_test_mold_entries (struct disir_instance *instance,
                       struct disir_register_plugin *plugin, struct disir_entry **entries)
{
    struct disir_entry *queue;
    struct disir_entry *entry;

    (void) &instance;
    (void) &plugin;


    queue = NULL;

    for (auto i = molds.begin(); i != molds.end(); ++i)
    {
        entry  = (struct disir_entry *) calloc (1, sizeof (struct disir_entry));
        if (entry == NULL)
            continue;

        entry->de_entry_name = strdup (i->first.c_str());
        entry->flag.DE_READABLE = 1;
        entry->flag.DE_WRITABLE = 1;
        if (i->first.back() == '/')
        {
            entry->flag.DE_NAMESPACE_ENTRY = 1;
        }
        MQ_ENQUEUE (queue, entry);
    }

    *entries = queue;

    return DISIR_STATUS_OK;
}

enum disir_status
dio_test_mold_query (struct disir_instance *instance, struct disir_register_plugin *plugin,
                     const char *entry_id, struct disir_entry **entry)
{
    char namespace_entry[PATH_MAX];

    (void) &instance;
    (void) &plugin;

    if (entry_id == NULL)
        return DISIR_STATUS_INTERNAL_ERROR;;

    std::string name(entry_id);
    if (molds.count (name) == 0)
    {
        if (fslib_namespace_entry (entry_id, namespace_entry) == NULL)
            return DISIR_STATUS_NOT_EXIST;

        name = std::string (namespace_entry);
        // The namespace entry does not exist
        if (molds.count (name) == 0)
        {
            return DISIR_STATUS_NOT_EXIST;
        }
    }

    if (entry != NULL)
    {
        *entry  = (struct disir_entry *) calloc (1, sizeof (struct disir_entry));
        if (*entry == NULL)
            return DISIR_STATUS_NO_MEMORY;

        (*entry)->de_entry_name = strdup (name.c_str());
        (*entry)->flag.DE_READABLE = 1;
        (*entry)->flag.DE_WRITABLE = 1;
        if (name.back() == '/')
        {
            (*entry)->flag.DE_NAMESPACE_ENTRY = 1;
        }
    }

    return DISIR_STATUS_EXISTS;
}

