#ifndef _LIBDISIR_UTIL_H
#define _LIBDISIR_UTIL_H

#include <disir/disir.h>

//! \struct semantic_version provides a dedicated data type 
//! to represent semantically versioned objects.
struct semantic_version
{
    uint32_t    sv_major;
    uint32_t    sv_minor;
    uint32_t    sv_patch;
};

//! \return return if < 0 if s1 is lesser than s2,
//! \return return 0 if s1 == s2
//! \return return > 0 if s2 is greater than s1.
int dx_semantic_version_compare(struct semantic_version *s1, struct semantic_version *s2);

//! Populate the buffer, up to buffer_size bytes, with a string representation
//! of the semantic_version number passed.
//! \return buffer upon successful copy.
//! \return string indicating the error upon insufficient buffer or arguments.
char * dx_semver_string(char *buffer, int32_t buffer_size, struct semantic_version *semver);


#endif // _LIBDISIR_UTIL_H
