#ifndef _LIBDISIR_UTIL_H
#define _LIBDISIR_UTIL_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus


#include <disir/disir.h>

//! \brief Structure represents a disir version number.
//!
//! A semantic version number is used to denote the version
//! of any key property within Disir, so their historic values
//! can be tracked and allow comparison between different versions.
//!
//! Only the major and minor part of the semantic version number is used.
//! A change in major version number indicate a semantic change that
//! in the core property that Disir is not equipt to resolve; this must
//! be handled by a third party.
//!
struct disir_version
{
    //! The major number component of a semantic version number.
    uint32_t    sv_major;
    //! The minor number component of a semantic version number.
    uint32_t    sv_minor;
};

//! \brief Populate the input buffer with a string representation of the semantic version structure
//!
//! If the input buffer is of unsufficient size, NULL is returned
//!
//! \return NULL if buffer or version are NULL.
//! \return NULL if buffer is of insufficient size.
//! \return buffer when the full string represention was populated into the buffer.
//!
DISIR_EXPORT
char *
dc_version_string (char *buffer, int32_t buffer_size,
                   struct disir_version *version);

//! \brief Extract a semantic version number from input string and populate the output version.
//!
//! \param[in] input String where the semantic version number is located
//! \param[out] version Version structure that is populated with the semantic version
//!     number found in the input string.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if input or version is NULL.
//! \return DISIR_STATUS_INVALID_ARGUMENT if input string does not contain the expected
//!     3 numeric numbers seperated by a period. version structure is partially updated
//!     with the values it has already detected and parsed.
//! \return DISRI_STATUS_OK on success
//!
DISIR_EXPORT
enum disir_status
dc_version_convert (const char *input, struct disir_version *version);

//! \brief Populate the destination version with the values of source version
//!
//! No input validation is performed.
//!
DISIR_EXPORT
void
dc_version_set (struct disir_version *destination, struct disir_version *source);

//! \brief Compare the two input semantic versions structures
//!
//! \return < 0 if s1 is lesser than s2,
//! \return 0 if s1 == s2
//! \return > 0 if s2 is greater than s1.
//!
DISIR_EXPORT
int
dc_version_compare (struct disir_version *s1, struct disir_version *s2);


#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_UTIL_H

