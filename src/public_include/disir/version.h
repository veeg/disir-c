#ifndef _LIBDISIR_VERSION_H
#define _LIBDISIR_VERSION_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

//! Major version of compiled libdisir.
extern const unsigned int libdisir_version_major;
//! Minor version of compiled libdisir.
extern const unsigned int libdisir_version_minor;
//! Patch version of compiled libdisir.
extern const unsigned int libdisir_version_patch;
//! String representation of the semantic version of the compiled libdisir
extern const char *libdisir_version_string;

//! Random build number assigned at compile time.
//! Could prove useful to identify different compilations of the same version
extern const char *libdisir_build_string;


#ifdef __cplusplus
}
#endif // __cplusplus

#endif //_LIBDISIR_VERSION_H

