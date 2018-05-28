#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <disir/disir.h>

#include "config.h"
#include "context_private.h"
#include "documentation.h"
#include "keyval.h"
#include "log.h"
#include "mold.h"
#include "multimap.h"
#include "restriction.h"
#include "section.h"

//! Forward declare
static enum disir_status
diff_compare_contexts_with_report (struct disir_context *lhs, struct disir_context *rhs,
                                   struct disir_diff_report *report);


// String hashing function for the multimap
// http://www.cse.yorku.ca/~oz/hash.html
static unsigned long djb2 (char *str)
{
    unsigned long hash = 5381;
    char c;
    while( (c = *str++) ) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

//! STATIC FUNCTION
static struct disir_diff_report *
dx_diff_report_create (void)
{
    struct disir_diff_report *report;

    report = calloc(1, sizeof (struct disir_diff_report));
    if (report == NULL)
        goto error;

    report->dr_internal_allocated = 20;
    report->dr_entries = 0;

    report->dr_diff_string = calloc (report->dr_internal_allocated, sizeof (char **));
    if (report->dr_diff_string == NULL)
        goto error;

    return report;
error:
    if (report->dr_diff_string)
        free (report->dr_diff_string);
    if (report)
        free (report);

    return NULL;
}

//! STATIC FUNCTION
static void
dx_diff_report_destroy (struct disir_diff_report **report)
{
    int i = 0;

    for (i = 0; i < (*report)->dr_entries; i++)
    {
        free ((*report)->dr_diff_string[i]);
    }
    free ((*report)->dr_diff_string);
    free (*report);
    *report = NULL;
}

//! STATIC FUNCTION
static void
dx_diff_report_add (struct disir_diff_report *report, const char *fmt, ...)
{
    va_list args;
    int i;
    int n = 128;
    int ret = 0;
    void *moved = NULL;

    if (report->dr_entries == report->dr_internal_allocated)
    {
        moved = realloc (report->dr_diff_string,
                         (report->dr_internal_allocated * 2) * sizeof (char **));
        if (moved != NULL)
        {
            report->dr_diff_string = moved;
            report->dr_internal_allocated *= 2;
        }
        else
        {
            log_error ("Failed to reallocate memory for diff report entry.");
            return;
        }
    }

    i = report->dr_entries;
    report->dr_diff_string[i] = malloc (n);
    if (report->dr_diff_string[i] == NULL)
    {
        log_error ("Failed to allocate memory for diff entry string.");
        return;
    }

    va_start (args, fmt);
    do
    {
        ret = vsnprintf (report->dr_diff_string[i], n, fmt, args);
        if (ret < 0)
        {
            // Encoding error.
            log_error ("vsnprintf encoding error. Failed to write diff report string.");
            break;
        }
        else if (ret >= n)
        {
            // Insufficient space.
            n *= 2;
            moved = realloc (report->dr_diff_string, n);
            if (moved)
            {
                report->dr_diff_string[i] = moved;
            }
            else
            {
                log_error ("Failed to realloc memory for diff report string.");
                break;
            }
        }
        else
        {
            // Success
            report->dr_diff_string[i][ret] = '\0';
            log_debug (1, "Diff report: %s", report->dr_diff_string[i]);
            report->dr_entries += 1;
            break;
        }

        // Retry after we have increasted allocated space
    } while (1);

    va_end (args);
}

//! STATIC FUNCTION
static enum disir_status
compare_default_queues (struct disir_default *lhs, struct disir_default *rhs,
                        struct disir_diff_report *report)
{
    // Both queues are of equal length
    if (lhs == NULL && rhs == NULL)
    {
        return DISIR_STATUS_OK;
    }

    // lhs has at least one additional default entry.
    if (lhs && rhs == NULL)
    {
        dx_diff_report_add (report, "rhs is missing default entry");
        return DISIR_STATUS_OK;
    }

    // rhs has at least one additional documentation entry.
    if (rhs && lhs == NULL)
    {
        dx_diff_report_add (report, "lhs is missing default entry");
        return DISIR_STATUS_OK;
    }

    // Check introduced version
    int res;
    res = dc_version_compare (&lhs->de_introduced, &rhs->de_introduced);
    if (res != 0)
    {
        char lhsbuf[100];
        char rhsbuf[100];
        dc_version_string (lhsbuf, 100, &lhs->de_introduced);
        dc_version_string (rhsbuf, 100, &rhs->de_introduced);
        dx_diff_report_add (report, "Default differ in version (%s vs %s)",
                                    lhsbuf, rhsbuf);

        return DISIR_STATUS_OK;
    }

    // Check value
    if (dx_value_compare (&lhs->de_value, &rhs->de_value) != 0)
    {
        char lhsbuf[100];
        char rhsbuf[100];
        dx_value_stringify (&lhs->de_value, 100, lhsbuf, NULL);
        dx_value_stringify (&rhs->de_value, 100, rhsbuf, NULL);

        dx_diff_report_add (report, "Default value differ ('%s' vs '%s')", lhsbuf, rhsbuf);
        return DISIR_STATUS_OK;
    }

    // recurse to next entry
    return compare_default_queues (lhs->next, rhs->next, report);
}

//! STATIC FUNCTION
static enum disir_status
compare_documentation_queues (struct disir_documentation *lhs, struct disir_documentation *rhs,
                              struct disir_diff_report *report)
{
    // Both queues are of equal length
    if (lhs == NULL && rhs == NULL)
    {
        return DISIR_STATUS_OK;
    }

    // lhs has at least one additional documentation entry.
    if (lhs && rhs == NULL)
    {
        dx_diff_report_add (report, "rhs is missing documentation entry");
        return DISIR_STATUS_OK;
    }

    // rhs has at least one additional documentation entry.
    if (rhs && lhs == NULL)
    {
        dx_diff_report_add (report, "lhs is missing documentation entry");
        return DISIR_STATUS_OK;
    }

    // Check introduced version
    int res;
    res = dc_version_compare (&lhs->dd_introduced, &rhs->dd_introduced);
    if (res != 0)
    {
        char lhsbuf[100];
        char rhsbuf[100];
        dc_version_string (lhsbuf, 100, &lhs->dd_introduced);
        dc_version_string (rhsbuf, 100, &rhs->dd_introduced);
        dx_diff_report_add (report, "Documentation differ in version (%s vs %s)",
                                    lhsbuf, rhsbuf);

        return DISIR_STATUS_OK;
    }

    // Check value
    if (dx_value_compare (&lhs->dd_value, &rhs->dd_value) != 0)
    {
        const char *doc1;
        const char *doc2;
        dx_value_get_string (&lhs->dd_value, &doc1, NULL);
        dx_value_get_string (&rhs->dd_value, &doc2, NULL);
        dx_diff_report_add (report, "Documentation string differ ('%s' vs '%s')", doc1, doc2);
        return DISIR_STATUS_OK;
    }

    // recurse to next entry
    return compare_documentation_queues (lhs->next, rhs->next, report);
}

//! STATIC FUNCTION
static enum disir_status
compare_restriction_queue (struct disir_restriction *lhs, struct disir_restriction *rhs,
                           struct disir_diff_report *report)
{
    enum disir_status status;
    int res;

    // Both queues are of equal length
    if (lhs == NULL && rhs == NULL)
    {
        return DISIR_STATUS_OK;
    }

    // lhs has at least one additional default entry.
    if (lhs && rhs == NULL)
    {
        dx_diff_report_add (report, "rhs is missing restriction entry");
        return DISIR_STATUS_OK;
    }

    // rhs has at least one additional documentation entry.
    if (rhs && lhs == NULL)
    {
        dx_diff_report_add (report, "lhs is missing restriction entry");
        return DISIR_STATUS_OK;
    }

    // Check introduced version
    res = dc_version_compare (&lhs->re_introduced, &rhs->re_introduced);
    if (res != 0)
    {
        char lhsbuf[100];
        char rhsbuf[100];
        dc_version_string (lhsbuf, 100, &lhs->re_introduced);
        dc_version_string (rhsbuf, 100, &rhs->re_introduced);
        dx_diff_report_add (report, "Restriction differ in introduced (%s vs %s)",
                                    lhsbuf, rhsbuf);

        return DISIR_STATUS_OK;
    }

    // Check deprecated version
    res = dc_version_compare (&lhs->re_deprecated, &rhs->re_deprecated);
    if (res != 0)
    {
        char lhsbuf[100];
        char rhsbuf[100];
        dc_version_string (lhsbuf, 100, &lhs->re_deprecated);
        dc_version_string (rhsbuf, 100, &rhs->re_deprecated);
        dx_diff_report_add (report, "Restriction differ in deprecated (%s vs %s)",
                                    lhsbuf, rhsbuf);

        return DISIR_STATUS_OK;
    }

    // Check type
    if (lhs->re_type != rhs->re_type)
    {
        dx_diff_report_add (report, "Restriction type differ (%s vs %s)",
                                    dc_restriction_enum_string (lhs->re_type),
                                    dc_restriction_enum_string (rhs->re_type));
        return DISIR_STATUS_OK;
    }

    // Check documentation
    status = compare_documentation_queues (lhs->re_documentation_queue,
                                           rhs->re_documentation_queue, report);
    if (status != DISIR_STATUS_OK)
    {
        return status;
    }

    // Assert value
    // This is not pretty...
    if (lhs->re_value_numeric != rhs->re_value_numeric
        || lhs->re_value_min != rhs->re_value_min
        || lhs->re_value_max != rhs->re_value_max
        || (lhs->re_value_string != NULL
            && rhs->re_value_string != NULL
            && strcmp (lhs->re_value_string, rhs->re_value_string) != 0))
    {
        dx_diff_report_add (report, "Restriction values differ... (printout NYI)");
        return DISIR_STATUS_OK;
    }

    return DISIR_STATUS_OK;
}

//! STATIC FUNCTION
static enum disir_status
compare_all_in_name (struct disir_context *lhs, struct disir_context *rhs, const char *name,
                     struct disir_diff_report *report)
{
    enum disir_status status;
    struct disir_collection *lhs_collection = NULL;
    struct disir_collection *rhs_collection = NULL;

    struct disir_context *lhs_entry = NULL;
    struct disir_context *rhs_entry = NULL;

    log_debug (1, "Comparing %s (%p vs %p)", name, lhs, rhs);

    status = dc_find_elements (lhs, name, &lhs_collection);
    if (status != DISIR_STATUS_OK)
    {
        // Something has really gone awry, since we queried the lhs for this name just
        // before entering this function..
        goto out;
    }
    status = dc_find_elements (rhs, name, &rhs_collection);
    if (status != DISIR_STATUS_OK)
    {
        if (status == DISIR_STATUS_NOT_EXIST)
        {
            log_debug (3, "rhs does not contain name entry: %s", name);
            dx_diff_report_add (report, "%s not found in rhs.", dx_context_name (lhs));
            status = DISIR_STATUS_OK;
        }
        goto out;
    }

    // Pop an entry froom LHS. Compare with a popped entry from RHS.
    // If LHS contains more entries, report it in loop
    // if RHS contains more entries, report it out of loop.
    do
    {
        status = dc_collection_next (lhs_collection, &lhs_entry);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            status = DISIR_STATUS_OK;
            break;
        }
        if (status != DISIR_STATUS_OK)
        {
            log_debug (1, "lhs failed to retrieve collection context: %s",
                          disir_status_string (status));
            goto out;
        }

        // Get rhs
        status = dc_collection_next (rhs_collection, &rhs_entry);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            status = DISIR_STATUS_OK;
            dx_diff_report_add (report, "%s contains extra entry.", dx_context_name (lhs));
            dc_putcontext (&lhs_entry);
            continue;
        }
        if (status != DISIR_STATUS_OK)
        {
            log_debug (1, "rhs failed to retrieve collection context: %s",
                          disir_status_string (status));
            goto out;
        }

        // Compare entries.
        status = diff_compare_contexts_with_report (lhs_entry, rhs_entry, report);
        if (status != DISIR_STATUS_OK)
        {
            goto out;
        }

        dc_putcontext (&lhs_entry);
        dc_putcontext (&rhs_entry);
    } while (1);

    do
    {
        status = dc_collection_next (rhs_collection, &rhs_entry);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            break;
        }
        if (status == DISIR_STATUS_OK)
        {
            // TODO: Report value - stringify?
            dx_diff_report_add (report, "%s missing entry.", dx_context_name (lhs));

            dc_putcontext (&rhs_entry);
            continue;
        }

        dc_putcontext (&rhs_entry);
        log_debug (1, "rhs collection next returned: %s", disir_status_string (status));
        break;
    } while (1);


    status = DISIR_STATUS_OK;
    // FALL-THROUGH
out:
    if (lhs_entry)
    {
        dc_putcontext (&lhs_entry);
    }
    if (rhs_entry)
    {
        dc_putcontext (&rhs_entry);
    }
    if (lhs_collection)
    {
        dc_collection_finished (&lhs_collection);
    }
    if (rhs_collection)
    {
        dc_collection_finished (&rhs_collection);
    }
    return status;
}

//! STATIC FUNCTION
static enum disir_status
compare_all_elements (struct disir_context *lhs, struct disir_context *rhs,
                      struct disir_diff_report *report)
{
    // Loop over all elements, match element for element by name
    // This will work for both config and mold
    // Element names already handled is marked in a map,
    // and are thus skipped if encountered again (this to compare multiple entries in config)
    enum disir_status status;
    struct disir_collection *lhs_elements;
    struct disir_collection *rhs_elements;
    struct disir_context *element;
    const char *name;
    struct multimap *map;

    element = NULL;
    lhs_elements = NULL;
    rhs_elements = NULL;
    name = NULL;

    // Create a multimap
    map = multimap_create ((int (*)(const void *, const void*)) strcmp,
                           (unsigned long (*)(const void*)) djb2);
    if (map == NULL)
    {
        log_debug (1, "failed to allocate multimap for comparison");
        status = DISIR_STATUS_NO_MEMORY;
        goto out;
    }

    status = dc_get_elements (lhs, &lhs_elements);
    if (status != DISIR_STATUS_OK)
    {
        log_debug (1, "failed to get elements from lhs: %s", disir_status_string (status));
        goto out;
    }

    do
    {
        status = dc_collection_next (lhs_elements, &element);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            status = DISIR_STATUS_OK;
            break;
        }
        if (status != DISIR_STATUS_OK)
        {
            log_debug (1, "Failed to get collection item: %s", disir_status_string (status));
            break;
        }

        log_debug (5, "Getting name for element: %p", element);
        // Get name
        status = dc_get_name (element, &name, NULL);
        if (status != DISIR_STATUS_OK)
        {
            log_debug (1, "failed to get name: %s", disir_status_string (status));
            break;
        }

        dc_putcontext (&element);

        // Entry already handled. Skip it.
        if (multimap_contains_key (map, name))
        {
            dc_putcontext (&element);
            continue;
        }

        // Mark name as already handled
        multimap_push_value (map, name, (void *)1);

        // Kick off a sub-search between lhs and rhs on name.
        // Store already mapped entries in map.
        status = compare_all_in_name (lhs, rhs, name, report);
        if (status != DISIR_STATUS_OK)
        {
            break;
        }
    } while (1);

    // Map now contains all entries that exists in lhs.
    // We need to iterate rhs to check if there resides any entries there
    // that is not part of map, e.g., not part of lhs.

    status = dc_get_elements (rhs, &rhs_elements);
    if (status != DISIR_STATUS_OK)
    {
        // QUESTION: Could there be a valid scenario where rhs is empty?
        log_debug (1, "failed to get elements from rhs: %s", disir_status_string (status));
        goto out;
    }

    do
    {
        status = dc_collection_next (rhs_elements, &element);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            status = DISIR_STATUS_OK;
            break;
        }
        if (status != DISIR_STATUS_OK)
        {
            log_debug (1, "Failed to get collection item: %s", disir_status_string (status));
            break;
        }

        log_debug (5, "Getting name for element: %p", element);
        // Get name
        status = dc_get_name (element, &name, NULL);
        if (status != DISIR_STATUS_OK)
        {
            log_debug (1, "failed to get name: %s", disir_status_string (status));
            break;
        }

        dc_putcontext (&element);

        // Entry exists in lhs - skip it
        if (multimap_contains_key (map, name))
        {
            dc_putcontext (&element);
            continue;
        }

        // rhs contains extra entry not present in lhs
        dx_diff_report_add (report, "rhs contains entry '%s' not present in lhs.", name);
    } while (1);

    // Cleanup
    if (map)
    {
        multimap_destroy (map, NULL, NULL);
    }
    if (lhs_elements)
    {
        dc_collection_finished (&lhs_elements);
    }
    if (rhs_elements)
    {
        dc_collection_finished (&rhs_elements);
    }

    if (element)
    {
        dc_putcontext (&element);
    }

    // FALL-THROUGH
out:
    return status;
}

//! STATIC FUNCTION
//! Only return non-OK status on exceptional condition.
//! If lhs and rhs differ, enter the difference into the diff_report and return OK.
static enum disir_status
diff_compare_contexts_with_report (struct disir_context *lhs, struct disir_context *rhs,
                                   struct disir_diff_report *report)
{
    enum disir_status status;

    status = DISIR_STATUS_OK;
    // QUESTION: Should we resolve names recursively to parent?
    //  This will allow us to easier identify by name what differs to what.

    log_debug (5, "Diff compare contexts (%p vs %p)", lhs, rhs);

    // Compare root contet type
    if (lhs->cx_root_context->cx_type != rhs->cx_root_context->cx_type)
    {
        log_debug (3, "lhs and rhs root context type differs (%s vs %s)",
                       dc_context_type_string (lhs->cx_root_context),
                       dc_context_type_string (rhs->cx_root_context));
        dx_diff_report_add (report, "%s root type is %s, %s root is of different type %s.",
                                    dx_context_name (lhs),
                                    dc_context_type_string (lhs->cx_root_context),
                                    dx_context_name (rhs),
                                    dc_context_type_string (rhs->cx_root_context));
        return DISIR_STATUS_OK;
    }

    // Compare context type
    if (lhs->cx_type != rhs->cx_type)
    {
        log_debug (3, "lhs and rhs context type differs (%s vs %s)",
                       dc_context_type_string (lhs),
                       dc_context_type_string (rhs));
        dx_diff_report_add (report, "%s is of type %s, %s is of different type %s.",
                                    dx_context_name (lhs), dc_context_type_string (lhs),
                                    dx_context_name (rhs), dc_context_type_string (rhs));
        return DISIR_STATUS_OK;
    }

    // Compare value type
    if (dc_value_type (lhs) != dc_value_type (rhs))
    {
        log_debug (3, "lhs and rhs value type differs (%s vs %s)",
                       dc_value_type_string (lhs),
                       dc_value_type_string (rhs));

        dx_diff_report_add (report, "%s is of value type %s, %s is of different value type %s.",
                                    dx_context_name (lhs), dc_value_type_string (lhs),
                                    dx_context_name (rhs), dc_value_type_string (rhs));
        return DISIR_STATUS_OK;
    }

    switch (lhs->cx_type)
    {
        case DISIR_CONTEXT_KEYVAL:
    {
        if (dx_value_compare (&lhs->cx_keyval->kv_value, &rhs->cx_keyval->kv_value) != 0)
        {
            int32_t size;
            char lhsbuf[100];
            char rhsbuf[100];
            dx_value_stringify (&lhs->cx_keyval->kv_value, 100, lhsbuf, &size);
            dx_value_stringify (&rhs->cx_keyval->kv_value, 100, rhsbuf, &size);
            // QUESTION: Remove hardcoded size and allow dynamic range?
            dx_diff_report_add (report, "%s differs in value from %s. ('%s' vs '%s')",
                                        dx_context_name (lhs), dx_context_name (rhs),
                                        lhsbuf, rhsbuf);
            log_debug (3, "lhs and rhs value differs: (%s vs %s)", lhsbuf, rhsbuf);
        }
        else
        {
            log_debug (3, "lhs and rhs value same value! REJOICE!");
        }

        if (dc_context_type (lhs->cx_root_context) == DISIR_CONTEXT_MOLD)
        {
            status = compare_documentation_queues (lhs->cx_keyval->kv_documentation_queue,
                                                   rhs->cx_keyval->kv_documentation_queue,
                                                   report);
            if (status != DISIR_STATUS_OK)
            {
                break;
            }

            status = compare_default_queues (lhs->cx_keyval->kv_default_queue,
                                             rhs->cx_keyval->kv_default_queue,
                                             report);
            if (status != DISIR_STATUS_OK)
            {
                break;
            }

            status = compare_restriction_queue (lhs->cx_keyval->kv_restrictions_queue,
                                                rhs->cx_keyval->kv_restrictions_queue,
                                                report);
            if (status != DISIR_STATUS_OK)
            {
                break;
            }
        }

        break;
    }
    case DISIR_CONTEXT_MOLD:
    {
        status = compare_documentation_queues (lhs->cx_mold->mo_documentation_queue,
                                               rhs->cx_mold->mo_documentation_queue,
                                               report);
        if (status != DISIR_STATUS_OK)
        {
            break;
        }

        status = compare_all_elements (lhs, rhs, report);
        break;
    }
    case DISIR_CONTEXT_CONFIG:
    {
        // TODO CONFIG: Compare version?

        status = compare_all_elements (lhs, rhs, report);
        break;
    }
    case DISIR_CONTEXT_SECTION:
    {
        // TODO MOLD: Diff restrictions

        // For mold, check documentation entries
        if (dc_context_type (lhs->cx_root_context) == DISIR_CONTEXT_MOLD)
        {
            status = compare_documentation_queues (lhs->cx_section->se_documentation_queue,
                                                   rhs->cx_section->se_documentation_queue,
                                                   report);
            if (status != DISIR_STATUS_OK)
            {
                break;
            }

            status = compare_restriction_queue (lhs->cx_section->se_restrictions_queue,
                                                rhs->cx_section->se_restrictions_queue,
                                                report);
            if (status != DISIR_STATUS_OK)
            {
                break;
            }
        }

        status = compare_all_elements (lhs, rhs, report);
        break;
    }
    default:
    {
        // Ignore default case?
        status = DISIR_STATUS_NOT_SUPPORTED;
    }
    }

    return status;
}

//! PUBLIC API
enum disir_status
dc_compare (struct disir_context *lhs, struct disir_context *rhs,
            struct disir_diff_report **report)
{
    enum disir_status status;
    struct disir_diff_report *internal_report;

    if (lhs == NULL || rhs == NULL)
    {
        log_debug (0, "invoked with NULL pointer(s). (lhs %p, rhs %p)");
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    internal_report = dx_diff_report_create ();
    if (internal_report == NULL)
    {
        log_debug (0, "memory allocation failed for disir diff_report");
        return DISIR_STATUS_NO_MEMORY;
    }

    status = diff_compare_contexts_with_report (lhs, rhs, internal_report);
    if (status == DISIR_STATUS_OK && internal_report->dr_entries > 0)
    {
        status = DISIR_STATUS_CONFLICT;
    }
    if (internal_report->dr_entries == 0)
    {
        dx_diff_report_destroy (&internal_report);
    }

    // Is the caller interested in our report?
    if (report)
    {
        *report = internal_report;
    }
    else if (internal_report)
    {
        dx_diff_report_destroy (&internal_report);
    }

    return status;
}

