#ifndef _LIBDISIR_CONTEXT_CONFIG_H
#define _LIBDISIR_CONTEXT_CONFIG_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus


//!
//! This file exposes the low level Disir Config Context API.
//!


//! \brief Retrieve the context associated with an already constructed disir_config.
//!
//! This context may be used to manipulate or query the config object.
//! The caller is responsible for invoking dc_putcontext()
//! after all operations are finished.
//!
//! \return NULL if config is NULL.
//! \return context of type DISIR_CONTEXT_CONFIG.
//!
struct disir_context *
dc_config_getcontext (struct disir_config *config);

//! \brief Get the version number of this config.
//!
//! \param[in] config Input mold to retrieve version for
//! \param[out] version Output structure populated with the version of config.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if config or version are NULL
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_config_get_version (struct disir_config *config, struct disir_version *version);

//! \brief Begin construction of a CONFIG context based on the passed mold.
//!
//! \param[in] mold Input mold that this CONFIG object shall represent.
//! \param[out] config Output CONFIG context object.
//!
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_config_begin (struct disir_mold *mold, struct disir_context **config);

//! \brief Finalize the construction of a DISIR_CONTEXT_CONFIG
//!
//! \param[in,out] context A CONFIG context to finalize. Will be sat to NULL on success.
//! \param[out] config The CONFIG object to populated on success.
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if input context is not of type CONFIG.
//! \return DISIR_STATUS_INVALID_CONTEXT if any part of the context is not valid.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dc_config_finalize (struct disir_context **context,
                                      struct disir_config **config);

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

//! \brief Query the config for an enum valued keyval.
//!
//! For an exhaustive explaination of the query syntax, see XXX_QUERY_XXX
//!
//! \param[in] context The parent config/section to begin name resolution from.
//! \param[out] value The pointer to populate a reference to the enum value to retrieve.
//! \param[in] query The varadic template and arguments to construct the query. See XXX_QUERY_XXX.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type enum.
//!
enum disir_status
dc_config_get_keyval_enum (struct disir_context *context, const char **value,
                           const char *query, ...);

//! \brief Set a enum keyval in config.
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] context The parent config/section to begin name resolution from.
//! \param[in] value The value to set the queried enum keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type enum.
//!
enum disir_status
dc_config_set_keyval_enum (struct disir_context *context, const char *value,
                           const char *query, ...);

//! \brief Query the config for a boolean valued keyval.
//!
//! For an exhaustive explaination of the query syntax, see XXX_QUERY_XXX
//!
//! \param[in] context The parent config/section to begin name resolution from.
//! \param[out] value The pointer to populate a reference to the boolean value to retrieve.
//! \param[in] query The varadic template and arguments to construct the query. See XXX_QUERY_XXX.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type boolean.
//!
enum disir_status
dc_config_get_keyval_boolean (struct disir_context *context, uint8_t *value,
                              const char *query, ...);

//! \brief Set a boolean keyval in config.
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] context The parent config/section to begin name resolution from.
//! \param[in] value The value to set the queried boolean keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type boolean.
//!
enum disir_status
dc_config_set_keyval_boolean (struct disir_context *context, uint8_t value,
                              const char *query, ...);

//! \brief Query the config for a float valued keyval.
//!
//! For an exhaustive explaination of the query syntax, see XXX_QUERY_XXX
//!
//! \param[in] context The parent config/section to begin name resolution from.
//! \param[out] value The pointer to populate a reference to the float value to retrieve.
//! \param[in] query The varadic template and arguments to construct the query. See XXX_QUERY_XXX.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type float.
//!
enum disir_status
dc_config_get_keyval_float (struct disir_context *context, double *value,
                            const char *query, ...);

//! \brief Set a float keyval in config.
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] context The parent config/section to begin name resolution from.
//! \param[in] value The value to set the queried float keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type float.
//!
enum disir_status
dc_config_set_keyval_float (struct disir_context *context, double value,
                            const char *query, ...);

//! \brief Query the config for a integer valued keyval.
//!
//! For an exhaustive explaination of the query syntax, see XXX_QUERY_XXX
//!
//! \param[in] context The parent config/section to begin name resolution from.
//! \param[out] value The pointer to populate a reference to the integer value to retrieve.
//! \param[in] query The varadic template and arguments to construct the query. See XXX_QUERY_XXX.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type integer.
//!
enum disir_status
dc_config_get_keyval_integer (struct disir_context *context, int64_t *value,
                              const char *query, ...);

//! \brief Set a integer keyval in config.
//!
//! \see Detailed Disir query resolution semantic for setters.
//!
//! \param[in] context The parent config/section to begin name resolution from.
//! \param[in] value The value to set the queried integer keyval to.
//! \param[in] query A string suitable for query name resolution. See relevant documentation.
//! \param[in] ... Varadic argument list to populate query with, when applicable.
//!
//! \return DISIR_STATUS_OK on success.
//! \return DISIR_STATUS_WRONG_VALUE_TYPE if the resolved query is not of type integer.
//!
enum disir_status
dc_config_set_keyval_integer (struct disir_context *context, int64_t value,
                              const char *query, ...);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_CONTEXT_CONFIG_H

