#ifndef _LIBDISIR_PRIVATE_FS_H
#define _LIBDISIR_PRIVATE_FS_H

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


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_PRIVATE_CONFIG_H

