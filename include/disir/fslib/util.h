#ifndef _LIBDISIR_FSLIB_UTIL_H
#define _LIBDISIR_FSLIB_UTIL_H

#include <disir/disir.h>
#include <string.h>

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

//! Create the input path recursively
//! Similar to shall command mkdir -p
//!
//! \return DISIR_STATUS_PERMISSION_ERROR on EACCES
//! \return DISIR_STATUS_FS_ERROR on any other error condition
//! \return DISIR_STATUS_OK on success (entire path may already exist as directories)
//!
enum disir_status
fslib_mkdir_p (struct disir_instance *instance, const char *path);


//! Create namespace entry of input name
//!
//! return empty string if no such namespace entry can be created
//!
const char *
fslib_namespace_entry (const char *name, char *namespace_entry);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_FSLIB_UTIL_H

