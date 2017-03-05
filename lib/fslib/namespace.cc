
#include <disir/fslib/util.h>

#include <string.h>

// FSLIB API
const char *
fslib_namespace_entry (const char *name, char *namespace_entry)
{
    namespace_entry[0] = '\0';

    // We must check if this covered by a namespace entry
    // Remove everything after the last /
    const char *found = strrchr (name, '/');
    if (found == NULL)
        return NULL;

    memcpy (namespace_entry, name, found - name + 1);
    namespace_entry[found - name + 1] = '\0';

    return namespace_entry;
}
