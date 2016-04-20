#ifndef _LIBDISIR_UTIL_PRIVATE_H
#define _LIBDISIR_UTIL_PRIVATE_H

//! \brief Populate the destination semver with the values of source semver
//!
//! No input validation is performed.
void
dx_semantic_version_set (struct semantic_version *destination, struct semantic_version *source);

//! \brief Compare the two input semantic versions structures
//!
//! \return < 0 if s1 is lesser than s2,
//! \return 0 if s1 == s2
//! \return > 0 if s2 is greater than s1.
//!
int dx_semantic_version_compare (struct semantic_version *s1, struct semantic_version *s2);


#endif // _LIBDISIR_UTIL_PRIVATE_H

