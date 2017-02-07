
//!
//! This file implements the entire test catalog, by implementing all functions
//! required for a disir plugin that may serve configs generated from mold, and molds
//! defined programmatically. This is part of the extended disir library so that
//! it is easy to iterate and test new plugins.
//!

#include <map>
#include <utility>
#include <string.h>

#include <disir/disir.h>
#include <disir/util.h>
#include <disir/plugin.h>
#include <disir/test.h>

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

typedef enum disir_status (*output_mold)(struct disir_mold **);

struct cmp_str
{
   bool operator()(char const *a, char const *b)
   {
      return strcmp(a, b) < 0;
   }
};

//! Internal static map to store test molds by id
static std::map<const char *, output_mold, cmp_str> molds = {
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
};

enum disir_status
dio_test_config_read (struct disir_instance *instance,
                      struct disir_plugin *plugin, const char *entry_id,
                      struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    output_mold func_mold;

    func_mold = molds[entry_id];
    if (func_mold == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

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
                         struct disir_plugin *plugin, struct disir_entry **entries)
{
    return dio_test_mold_entries (instance, plugin, entries);
}

enum disir_status
dio_test_config_query (struct disir_instance *instance,
                       struct disir_plugin *plugin, const char *entry_id)
{
    if (molds[entry_id] == NULL)
        return DISIR_STATUS_NOT_EXIST;
    else
        return DISIR_STATUS_EXISTS;
}

enum disir_status
dio_test_mold_read (struct disir_instance *instance, struct disir_plugin *plugin,
                    const char *entry_id, struct disir_mold **mold)
{
    enum disir_status status;
    output_mold func_mold;

    func_mold = molds[entry_id];
    if (func_mold == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    status = func_mold (mold);
    if (status != DISIR_STATUS_OK)
        return status;

    return DISIR_STATUS_OK;;
}

enum disir_status
dio_test_mold_entries (struct disir_instance *instance,
                       struct disir_plugin *plugin, struct disir_entry **entries)
{
    struct disir_entry *queue;
    struct disir_entry *entry;

    queue = NULL;

    for (auto i = molds.begin(); i != molds.end(); ++i)
    {
        entry  = (struct disir_entry *) calloc (1, sizeof (struct disir_entry));
        if (entry == NULL)
            continue;

        entry->de_entry_name = strdup (i->first);
        entry->DE_READABLE = 1;
        entry->DE_WRITABLE = 1;
        MQ_ENQUEUE (queue, entry);
    }

    *entries = queue;

    return DISIR_STATUS_OK;
}

enum disir_status
dio_test_mold_query (struct disir_instance *instance,
                     struct disir_plugin *plugin, const char *entry_id)
{
    if (molds[entry_id] == NULL)
        return DISIR_STATUS_NOT_EXIST;
    else
        return DISIR_STATUS_EXISTS;
}

