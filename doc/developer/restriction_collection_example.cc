#include <disir/disir.h>
#include <stdio.h>

void
print_restriction_collection (struct disir_context *context)
{
    enum disir_status status;
    enum disir_restriction_type restriction_type;
    enum disir_value_type value_type;
    struct disir_collection *collection;
    struct disir_context *restriction;

    status = dc_restriction_collection (context, &collection);
    if (status != DISIR_STATUS_OK && status != DISIR_STATUS_NOT_EXIST)
    {
        fprintf (stderr, "Error querying context for restrictions: %s",
                         disir_status_string (status));
        return;
    }
    if (status == DISIR_STATUS_NOT_EXIST)
    {
        fprintf (stdout, "Context has no restrictions\n");
        return;
    }

    do
    {
        status = dc_collection_next (collection, &restriction);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            break;
        }

        if (status != DISIR_STATUS_OK)
        {
            fprintf (stderr, "Error querying collection: %s", disir_status_string (status));
            break;
        }

        dc_get_restriction_type (restriction, &restriction_type);
        dc_get_value_type (context, &value_type);

        // Print version applicability range
        char buffer1[100];
        char buffer2[100];
        struct semantic_version introduced;
        struct semantic_version deprecated;
        dc_get_introduced (restriction, &introduced);
        dc_get_deprecated (restriction, &deprecated);
        fprintf (stdout, "(%s - %s) ",
                         dc_semantic_version_string (buffer1, 100, &introduced),
                         dc_semantic_version_string (buffer2, 100, &deprecated));

        // Print type
        fprintf (stdout, "[%s] ", dc_restriction_enum_string (restriction_type));

        // Print value
        switch (restriction_type)
        {
        case DISIR_RESTRICTION_INC_ENTRY_MIN:
        case DISIR_RESTRICTION_INC_ENTRY_MAX:
            value_type = DISIR_VALUE_TYPE_INTEGER;
        case DISIR_RESTRICTION_EXC_VALUE_NUMERIC:
        {
            double value;

            dc_restriction_get_numeric (restriction, &value);
            if (value_type == DISIR_VALUE_TYPE_FLOAT)
            {
                fprintf (stdout, "%f", value);
            }
            else
            {
                fprintf (stdout, "%ld", (long long)value);
            }
            break;
        }
        case DISIR_RESTRICTION_EXC_VALUE_RANGE:
        {
            double min, max;

            dc_restriction_get_range (restriction, &min, &max);
            if (value_type == DISIR_VALUE_TYPE_FLOAT)
            {
                fprintf (stdout, "%f - %f", min, max);
            }
            else
            {
                fprintf (stdout, "%ld - %ld", (long long)min, (long long)max);
            }
            break;
        }
        case DISIR_RESTRICTION_EXC_VALUE_ENUM:
        {
            const char *value= NULL;
            dc_restriction_get_string (context, &value);
            fprintf (stdout, "%s", value);
            break;
        }
        default:
        {
            // Ignore
            break;
        }
        }

        fprintf (stdout, "\n");
        dc_putcontext (&restriction);
    } while (1);

    // in case something bad happend.
    if (restriction)
        dc_putcontext (&restriction);

    dc_collection_finished (&collection);
}

