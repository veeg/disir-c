#ifndef _LIBDISIR_CONTEXT_CONFIG_H
#define _LIBDISIR_CONTEXT_CONFIG_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <disir/disir.h>

//!
//! This file exposes the low level Disir Config Context API.
//!

//! \brief Query the context for a heirarchical string keyval child.
//!
//! \param[in] context The parent config/section to begin name resolution from.
//! \param[out] value The pointer to populate the reference to the string value to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_config_get_keyval_string (struct disir_context *context, const char **value,
                             const char *query, ...);

//! \brief Set the keyval string value
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] context The parent config/section to begin name resolution from.
//! \param[in] value The value to set the queried string keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_config_set_keyval_string (struct disir_context *parent, const char *value,
                             const char *name, ...);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_CONTEXT_CONFIG_H

