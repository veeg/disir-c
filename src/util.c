// Standard includes
#include <stdlib.h>
#include <stdio.h>

// Public disir interface
#include <disir/disir.h>

// Private disir includes
#include "log.h"

//! PUBLIC API
char *
dx_semver_string(char *buffer, int32_t buffer_size, struct semantic_version *semver)
{
    int res;
    if (buffer == NULL || semver == NULL)
    {
        log_warn("%s invoked with NULL buffer or semverpointer", __FUNCTION__);
        return "<invalid_arguments>";
    }

    res = snprintf(buffer, buffer_size, "%d.%d.%d.", semver->sv_major, semver->sv_minor, semver->sv_patch);
    if (res < 0 || res >= buffer_size)
    {
        log_warn("Insufficient buffer( %p ) size( %d ) to copy semver (%d.%d.%d) - Res: %d", 
                    buffer, buffer_size, semver->sv_major, semver->sv_minor, 
                    semver->sv_patch, res);
        return "<insufficient_buffer>";
    }

    return buffer;
}

//! PUBLIC API
int
dx_semantic_version_compare(struct semantic_version *s1, struct semantic_version *s2)
{
    int res;
    
    res = 0;
    
    res = s1->sv_major - s2->sv_major;
    if (res == 0)
        res = s1->sv_minor - s2->sv_minor;
    if (res == 0)
        res = s1->sv_patch - s2->sv_patch;
    
    return res;
}