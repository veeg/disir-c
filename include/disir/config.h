#ifndef _LIBDISIR_CONFIG_H
#define _LIBDISIR_CONFIG_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#include <disir/disir.h>

//! \file config.h
//!
//! This file exposes the high level Disir Config API.
//!

//! \brief Query the config for a string valued keyval.
//!
//! For an exhaustive explaination of the query syntax, see XXX_QUERY_XXX
//!
//! \param[in] config The config object to query from.
//! \param[out] value The pointer to populate a reference to the string value to retrieve.
//! \param[in] query The varadic template and arguments to construct the query. See XXX_QUERY_XXX.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_config_get_keyval_string (struct disir_config *config, const char **value,
                                const char *query, ...);

//! \brief Set a string keyval in config.
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] config The config to query from.
//! \param[in] value The value to set the queried string keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
disir_config_set_keyval_string (struct disir_config *config, const char *value,
                                const char *query, ...);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_CONFIG_H

