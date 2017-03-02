#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

#include <disir/disir.h>

#include "fs.h"

//! STATIC API
static enum disir_status
handle_mkdir_errno (struct disir_instance *instance, const char *path, int errsave)
{
    if (errsave == EACCES)
    {
        disir_error_set (instance, "Insufficient access creating directory %s", path);
        return DISIR_STATUS_PERMISSION_ERROR;
    }
    else if (errsave != EEXIST)
    {
        // TODO: Use threadsafe strerror variant
        disir_error_set (instance, "Error creating directory %s: %s",
                         path, strerror (errsave));
        return DISIR_STATUS_FS_ERROR;
    }

    return DISIR_STATUS_OK;
}

//! FSLIB API
enum disir_status
fslib_mkdir_p (struct disir_instance *instance, const char *path)
{
    enum disir_status status;
    const size_t len = strlen (path);
    char _path[PATH_MAX];
    char *p;

    errno = 0;

    // Copy a mutable string
    if (len > sizeof(_path)-1)
    {
        errno = ENAMETOOLONG;
        disir_error_set (instance, "Supplied filepath to ensure recursive directories" \
                                    " exists exceed PATH_MAX (%d vs %d)", len, PATH_MAX);
        return DISIR_STATUS_INSUFFICIENT_RESOURCES;
    }
    strcpy(_path, path);

    // Iterate the string - make sure that every subentry exists
    for (p = _path + 1; *p; p++)
    {
        if (*p == '/')
        {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0)
            {
                status = handle_mkdir_errno (instance, _path, errno);
                if (status != DISIR_STATUS_OK)
                    return status;
            }

            *p = '/';
        }
    }

    // mkdir the entire path.
    if (mkdir(_path, S_IRWXU) != 0)
    {
        status = handle_mkdir_errno (instance, _path, errno);
    }

    return status;
}

