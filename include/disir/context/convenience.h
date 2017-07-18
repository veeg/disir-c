#ifndef _LIBDISIR_CONTEXT_CONVENIENCE_H
#define _LIBDISIR_CONTEXT_CONVENIENCE_H

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus


//!
//! This file contains the high level context convenience functions.
//! These are all implemented on top of the low level API endpoints.
//!

//! \brief Add a documentation string to an entry.
//!
//! This will have the default introduced version.
//! This is a shortcut between opening a new context,
//! adding value before finalizing it.
//!
//! Supported contexts:
//!     * KEYVAL
//!     * GROUP
//!     * CONFIG
//!     * MOLD
//!     * RESTRICTION
//!
//! \param context The input context to add documentation to.
//! \param doc The documentation string
//! \param doc_size The size of the `doc` string.
//!
//! \return DISIR_STATUS_WRONG_CONTEXT if an unsupported context is provided.
//! \return DISIR_STATUS_EXISTS  if a documentation entry already exists.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_add_documentation (struct disir_context *context, const char *doc, int32_t doc_size);

//! \brief Get the documentation entry for a given version on the context.
//!
//! NOTE: This is currently the only method for retrieving the documentation
//!     of a context.
//!
//! Retrieve a specific documentation entry valid for the input version number
//! given. If version is NULL, the highest version is picked.
//! Supported context types are:
//!     * DISIR_CONTEXT_SECTION
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_CONFIG
//!     * DISIR_CONTEXT_MOLD
//!     * DISIR_CONTEXT_RESTRICTION
//!
//! \param[in] context Input context to retrieve documentation for.
//! \param[in] version Matching documentation entry covered by this verison number.
//!     NULL indicates the greatest (version) documentation entry.
//! \param[out] doc Output pointer the documentation constant will be populated to.
//! \param[out] doc_size Size of the documentation string, in bytes. Optional; May be NULL
//!
//! \return DISIR_STATUS_INVALID_ARGUMENTS if context or doc are NULL
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not of supported type.
//! \return DISIR_STATUS_OK if doc is popualted with documentation string of context.
//!
enum disir_status
dc_get_documentation (struct disir_context *context, struct disir_version *version,
                      const char **doc, int32_t *doc_size);

//! \brief Shortcut to add a KEYVAL string entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output KEYVAL context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input version 'version'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status
dc_add_keyval_string (struct disir_context *parent,
                      const char *name, const char *def,
                      const char *doc, struct disir_version *version,
                      struct disir_context **output);


//! \brief Shortcut to add a KEYVAL enum entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! NOTE: The enum requires additional restrictions on it to mark valid enum values.
//! The context will be invalid after this call. One SHOULD catch the output context
//! and add the required restrictions
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output KEYVAL context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input version 'version'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status
dc_add_keyval_enum (struct disir_context *parent,
                    const char *name, const char *def,
                    const char *doc, struct disir_version *version,
                    struct disir_context **output);

//! \brief Shortcut to add a KEYVAL integer entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output KEYVAL context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input version 'version'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status
dc_add_keyval_integer (struct disir_context *parent,
                       const char *name, int64_t def,
                       const char *doc, struct disir_version *version,
                       struct disir_context **output);

//! \brief Shortcut to add a KEYVAL float entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output KEYVAL context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input version 'version'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status
dc_add_keyval_float (struct disir_context *parent,
                     const char *name, double def,
                     const char *doc, struct disir_version *version,
                     struct disir_context **output);

//! \brief Shortcut to add a KEYVAL boolean entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \param[out] output Optional output context storage. If address is passed,
//!             the output KEYVAL context is populated and reference increased.
//!             Caller must take care to use dc_putcontext () when he is done.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input version 'version'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status
dc_add_keyval_boolean (struct disir_context *parent,
                       const char *name, uint8_t def,
                       const char *doc, struct disir_version *version,
                       struct disir_context **output);

//! \brief  Add a default value to an entry, type inferred from the parent context
//!
//! Parse the input string to retrieve the value whose type is inferred from the
//! parent context. This function is merely a convenient wrapper around all
//! other dc_add_default_* functions.
//! The `value_size` parameter is only applicable for default entries whose
//! KEYVAL is STRING.
//!
//! \param context Parent context of which to add a new default entry
//! \param value String to parse the relevant value information from
//! \param value_size: Only applicable to string manipulating value types.
//!     Size in bytes of the input string to copy.A
//! \param version Number this default entry is valid from. NULL indicates
//!     the default version number.
//!
//! \return DISIR_STATUS_OK if a new default object was associated with the parent context
//!
enum disir_status
dc_add_default (struct disir_context *context, const char *value,
                int32_t value_size, struct disir_version *version);

//! \brief Add a default string value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_STRING. There must not be a default context
//! in parent with an equal version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_MOLD.
//! \param value Default string value to add.
//! \param value_size Size in bytes of the string to copy.
//! \param version Number this default entry is valid from. NULL indicates
//!     the default version number.
//!
//! \return DISIR_STATUS_OK if the default string 'value' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'version'.
//!
enum disir_status
dc_add_default_string (struct disir_context *parent, const char *value,
                       int32_t value_size, struct disir_version *version);

//! \brief Add a default integer value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_INTEGER. There must not be a default context
//! in parent with an equal version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_MOLD.
//! \param value Default integer value to add.
//! \param version Number this default entry is valid from. NULL indicates
//!     the default version number.
//!
//! \return DISIR_STATUS_OK if the default integer 'value' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'version'.
//!
enum disir_status
dc_add_default_integer (struct disir_context *parent, int64_t value,
                        struct disir_version *version);

//! \brief Add a default float value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_FLOAT. There must not be a default context
//! in parent with an equal version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_MOLD.
//! \param value Default float value to add.
//! \param version Number this default entry is valid from. NULL indicates
//!     the default version number.
//! \return DISIR_STATUS_OK if the default float 'value' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'version'.
//!
enum disir_status
dc_add_default_float (struct disir_context *parent, double value,
                      struct disir_version *version);

//! \brief Add a default boolean value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_BOOLEAN. There must not be a default context
//! in parent with an equal version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_MOLD.
//! \param booelan Default bool value to add.
//! \param version Number this default entry is valid from. NULL indicates
//!     the default version number.
//!
//! \return DISIR_STATUS_OK if the default 'boolean' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'version'.
//!
enum disir_status
dc_add_default_boolean (struct disir_context *parent, uint8_t boolean,
                        struct disir_version *version);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _LIBDISIR_CONTEXT_CONVENIENCE_H

