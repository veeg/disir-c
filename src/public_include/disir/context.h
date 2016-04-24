#ifndef _LIBDISIR_CONTEXT_H
#define _LIBDISIR_CONTEXT_H

#include <stdint.h>
#include <disir/disir.h>

//! The namespace employed for all low-level context operations
//! is dc_*

//! The disir_context is the main interface used to manipulate or
//! query various components within disir programatically.
//! It offers a detailed level of abstraction.
//! Each context is structured heirarchly, with the root
//! context being either DISIR_CONTEXT_CONFIG, DISIR_CONTEXT_SCHEMA
//! The root context will determine the
//! effect and allowed operations on child contexts.
struct disir_context;

//! Define a shortcut for struct disir_context, since it
//! is a often used construct.
typedef struct disir_context dc_t;


#include <disir/collection.h>

//! Different types of disir_contexts that are available.
enum disir_context_type
{
    DISIR_CONTEXT_CONFIG = 1,
    DISIR_CONTEXT_SCHEMA,
    DISIR_CONTEXT_SECTION,
    DISIR_CONTEXT_KEYVAL,
    DISIR_CONTEXT_DOCUMENTATION,
    DISIR_CONTEXT_DEFAULT,
    DISIR_CONTEXT_RESTRICTION,

    DISIR_CONTEXT_UNKNOWN, // Must be last entry in enumeration
};

//
// Utility context API
//

//! Return the disir_context_type associated with the passed
//! DISIR_CONTEXT.
enum disir_context_type dc_type (dc_t *context);

//! Return a string representation of the passed context.
//! type_string_size is populated with the size in octets for
//! the returned string. If this pointer is NULL, this output
//! parameter is ignored.
//! If context is NULL, the returned string is equal to
//! that if the input context were unknown.
const char * dc_type_string (dc_t *context);


//
// Base context API
//

//! Start the construction of a new context as a child of parent.
//! No state is altered in the parent until dc_finalize() is called.
//! \param parent is the parent context to which a child shall be added.
//! \param context_type determines the type of context for the child.
//!     Top-level contexts cannot be added as children of other contexts.
//! \return DISIR_STATUS_WRONG_CONTEXT if an unsupported context type is submitted.
//! \return DISIR_STATUS_OK when everything is OK!
enum disir_status dc_begin (dc_t *parent, enum disir_context_type context_type, dc_t **child);

//! Destroy the object pointed to by this context.
//! This will delete all children objects.
//! If any of the contexts, children or this one,
//! is referenced by any other pointers, the context will
//! turn INVALID and every operation attempted will result
//! in a DISIR_STATUS_INVALID_CONTEXT status code.
//! If a context pointer is INVALID, dc_destroy() must be invoked
//! on it to decrement the reference count.
enum disir_status dc_destroy (dc_t **context);

//! Submit all state built on the passed context to the parent.
//! On DISIR_STATUS_OK, context pointer is invalidated and set to NULL.
enum disir_status dc_finalize (dc_t **context);

//! When a context is not in 'constructing' mode,
//! any context returned is referenced counted.
//! When you are finished with a non-constructing mode context,
//! please put it back.
//! \return DISIR_STATUS_CONTEXT_IN_WRONG_STATE if context is not in constructing mode
//! \return DISIR_STATUS_OK when successful. Passed context pointer is set tp NULL.
enum disir_status dc_putcontext (dc_t **context);


//
// Documentation context API
//

//! Add a documentation string to an entry.
//! This will have the default introduced semver.
//! This is a shortcut between opening a new context,
//! adding value before finalizing it.
//!
//! Supported contexts:
//!     * KEYVAL
//!     * GROUP
//!     * CONFIG
//!     * SCHEMA
//! If an unsupported context is provided, DISIR_STATUS_WRONG_CONTEXT
//! is returned. If a documention entry already exists for this element,
//! DISIR_STATUS_EXISTS will be returned.
//! On success, DISIR_STATUS_OK is returned.
enum disir_status dc_add_documentation (dc_t *context, const char *doc, int32_t doc_size);

//! \brief Get the documentation entry for a given semver on the context.
//!
//! Retrieve a specific documentation entry valid for the input semantic version number
//! given. If semver is NULL, the highest version is picked.
//! Supported context types are:
//!     * DISIR_CONTEXT_SECTION
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_CONFIG
//!     * DISIR_CONTEXT_SCHEMA
//!
//! \param[in] context Input context to retrieve documentation for.
//! \param[in] semver Matching documentation entry covered by this semantic verison number.
//!     NULL indicates the greatest (semver) documentation entry.
//! \param[out] doc Output pointer the documentation constant will be populated to.
//! \param[out] doc_size Size of the documentation string, in bytes. Optional; May be NULL
//!
//! \return DISIR_STATUS_INVALID_ARGUMENTS if context or doc are NULL
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not of supported type.
//! \return DISIR_STATUS_OK if doc is popualted with documentation string of context.
//!
enum disir_status dc_get_documentation (dc_t *context, struct semantic_version *semver,
                                        const char **doc, int32_t *doc_size);

//
// Add related context API
//

//! \brief Add a name to a context entry.
//!
//! One can only add a name to a context who is in construction mode.
//! This is required on supported contexts:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!
//! \param context Context to set the name attribute on.
//! \param name The input name to associate with the context.
//! \param name_size Size in bytes of the input name. Does not include null terminator.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if name or name_size are zero
//! \return DISIR_STATUS_NO_CAN_DO if an unsupported context type
//! \return DISIR_STATUS_OK on successful insertion of name to context.
//!
enum disir_status dc_set_name (dc_t *context, const char *name, int32_t name_size);

//! \brief Get a name attribute associated with the context entry
//!
//! One can only retrieve a name from one of these supported contexts:
//!     * DISIR_CONTEXT_KEYVAL
//!     * DISIR_CONTEXT_SECTION
//!
//! \param[in] context Context to get name attribute from
//! \param[out] Pointer will be re-directed to the name constant
//! \param[out] Address is populated wth the output name size in bytes. May be NULL.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if context or name are NULL pointers.
//! \return DISIR_STATUS_WRONG_CONTEXT if input context is not of the supported types.
//! \return DISIR_STATUS_OK if name is successfully populated with the name attribute of context.
//!
enum disir_status dc_get_name (dc_t *context, const char **name, int32_t *name_size);

//! Add an introduced semantic version number to an entry.
//! One can only add an introduced semver to a context who is in construction mode.
//! Add a deprecrated semantic version number to an entry.
//! Depending on the context, checks for compatabilit with existing
//! elements in the parent may result in an errornous condition.
//! DISIR_STATUS_EXISTS is returned if an introduced entry already exists.
//! DISIR_STATUS_OK is returned on success.
enum disir_status dc_add_introduced(dc_t *context, struct semantic_version semver);
enum disir_status dc_add_deprecrated(dc_t *context, struct semantic_version smever);


//! \brief Set the value type associated with input context
//!
//! Set the value type of the input context.
//! The available input contexts are as follows:
//!     * DISIR_CONTEXT_KEYVAL
//!
//! There are a number of restrictions on when you can set a value type on a context.
//! DISIR_CONTEXT_KEYVAL: Cannot have any default entries on it.
//!     Can only set type if the toplevel context is DISIR_CONTEXT_SCHEMA
//!
//! \return DISIR_STATUS_OK the input 'context' was succesfuly populated with value type 'type'
//! \return DISIR_STATUS_INVALID_ARGUMENT if context is NULL or type is out-of-bounds
//!
enum disir_status dc_set_value_type (dc_t *context, enum disir_value_type type);

//! \brief Retrieve the value type stored in the input context
//!
//! Retrieve the value type of the input context, populated in the output argument 'type'
//!
//! \param[in] context The input context object to return the value type of.
//! \param[out] type Output value populated with the type found in context.
//!     DISIR_VALUE_TYPE_UNKNOWN is populated if any errors on the input context are found.
//!
//! \return DISIR_STATUS_OK if the context contatins a value type
//! \return DISIR_STATUS_WRONG_CONTEXT if it does not contain a value type.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either context or type are NULL pointer.
//!
enum disir_status dc_get_value_type (dc_t *context, enum disir_value_type *type);

//! \brief  Add a default value to an entry, type inferred from the parent context
//!
//! Parse the input string to retrieve the value whose type is inferred from the
//! parent context. This function is merely a convenient wrapper around all
//! other dc_add_default_* functions.
//!
//! \param context Parent context of which to add a new default entry
//! \param value String to parse the relevant value information from
//! \param value_size: Only applicable to string manipulating value types.
//!     Size in bytes of the input string to copy.A
//! \param semver Semantic version number to tag this default with. NULL indicates
//!     a semantic version number of 1.0.0
//!
//! \return DISIR_STATUS_OK if a new default object was associated with the parent context.t
enum disir_status dc_add_default (dc_t *context, const char *value,
                                  int32_t value_size, struct semantic_version *semver);

//! \brief Add a default string value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_STRING. There must not be a default context
//! in parent with an equal semantic version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_SCHEMA.
//! \param value Default string value to add.
//! \param value_size Size in bytes of the string to copy.
//! \param semver Semantic version number this default entry is valid from. NULL indicates
//!     the semantic verssion number 1.0.0
//!
//! \return DISIR_STATUS_OK if the default string 'value' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'semver'.
//!
enum disir_status dc_add_default_string (dc_t *parent, const char *value,
                                         int32_t value_size, struct semantic_version *semver);

//! \brief Add a default integer value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_INTEGER. There must not be a default context
//! in parent with an equal semantic version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_SCHEMA.
//! \param value Default integer value to add.
//! \param semver Semantic version number this default entry is valid from. NULL indicates
//!     the semantic verssion number 1.0.0
//!
//! \return DISIR_STATUS_OK if the default integer 'value' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'semver'.
//!
enum disir_status
dc_add_default_integer (dc_t *parent, int64_t value, struct semantic_version *semver);

//! \brief Add a default float value to the parent context.
//!
//! Type of parent must be DISIR_VALUE_TYPE_FLOAT. There must not be a default context
//! in parent with an equal semantic version number.
//!
//! \param parent A DISIR_CONTEXT_KEYVAL context, whose toplevel context is a DISIR_CONTEXT_SCHEMA.
//! \param value Default float value to add.
//! \param semver Semantic version number this default entry is valid from. NULL indicates
//!     the semantic verssion number 1.0.0
//!
//! \return DISIR_STATUS_OK if the default float 'value' entry succesfully added
//!     to the parent context.
//! \return DISIR_STATUS_CONFLICTING_SEMVER if there exists a default entry with equal 'semver'.
//!
enum disir_status
dc_add_default_float (dc_t *parent, double value, struct semantic_version *semver);

//! \brief Return the default value as a string representation from context.
//!
//! Retrieve the default value of input context as a string representation.
//! The supported contexts are:
//!     * DISIR_CONTEXT_DEFAULT: Plainly retrieve the appointed to default value
//!     * DISIR_CONTEXT_KEYVAL: Search for the matching default entry in keyval.
//! The matching default version when searching KEYVAL is picked. If semver is NULL,
//! the highest version is chosen.
//!
//! The output buffer is populated with the string representation of the default value held
//! by context. If the output_buffer_size is inssuficient in size, the output_string_size
//! will be equal or greater than output_buffer_size, and the output buffer populated
//! with a output_buffer_size - 1 bytes of data.
//! The output buffer is always NULL terminated.
//! On success, the output_string_size will always hold the exact number of bytes populated
//! in the buffer, not counting the terminating NULL character.
//!
//! \return DISIR_STATUS_INVALID_ARGUENT if context or output are NULL, or output_buffer_size
//!     is less than or equal to zero.
//! \return DISIR_STATUS_WRONG_CONTEXT if context is not of type DISIR_CONTEXT_DEFAULT or
//!     DISIR_CONTEXT_KEYVAL
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_get_default (dc_t *context, struct semantic_version *semver, int32_t output_buffer_size,
                char *output, int32_t *output_string_size);


//! \brief Gather all default entries on the context into a collection.
//!
//! The supported context for this function is the DISIR_CONTEXT_KEYVAL,
//! whose root context must be a DISIR_CONTEXT_SCHEMA.
//!
//! \param[in] context Input KEYVAL context to retrieve all default contexts from.
//! \param[out] collection Output collection populated with default contexts of input context.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if any of the input arguments are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if either the input context or root context are wrong.
//! \return DISIR_STATUS_NO_MEMORY if collection allocation failed.
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status dc_get_default_contexts (dc_t *context, dcc_t **collection);


//! Add a string as value to the context.
//! Not applicable to every context.
//! XXX: Make private?
enum disir_status dc_add_value_string (dc_t *context, const char *value, int32_t value_size);

//
// Query related context API
//

//! Return the number of bytes stored at the context string value, if applicable
enum disir_status dc_get_value_string_size (dc_t *context, int64_t *value_size);

//! Return the stored value of a context, along with the number of octets
//! contained in the returned valule.
enum disir_status dc_get_value_string (dc_t *context, char *value,
                                       int64_t output_buffer_maxsize, int64_t *value_size);
enum disir_status dc_get_value_integer (dc_t *context, int64_t *value);
enum disir_status dc_get_value_float (dc_t *conttext, double *value);

//! Query the context for its intrduced member.
//! If no such member exists on the context,
//! DISIR_STATUS_NO_CAN_DO is returned.
enum disir_status dc_get_introduced (dc_t *context, struct semantic_version *semver);

//! Query the context for its deprecrated member.
//! If no such member exists on the context,
//! DISIR_STATUS_NO_CAN_DO is returned.
enum disir_status dc_get_deprecrated (dc_t *context, struct semantic_version *semver);

//!  \brief Collect all direct child elements of the passed context.
//!
//! \param[in] context Parent context to collect child elements from.
//!     Must be of context ype:
//!         * DISIR_CONTEXT_CONFIG
//!         * DISIR_CONTEXT_SCHEMA
//!         * DISIR_CONTEXT_SECTION
//! \param[out] collection Output collection, if return status is DISIR_STATUS_OK
//!
//! \return DISIR_STATUS_OK if the output collection contains all
//!     child elements of a valid input context.
//! \return DISIR_STATUS_INVALID_ARGUMENT if input parameters are NULL
//! \return DISRI_STATUS_WRONG_CONTEXT if the input context is not of correct type.
//!
enum disir_status dc_get_elements (dc_t *context, dcc_t **collection);


//
// KEYVAL related context API
//

//! \brief Shortcut to add a KEYVAL string entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input semver 'semver'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status dc_add_keyval_string (dc_t *parent, const char *name, const char *def,
                                        const char *doc, struct semantic_version *semver);

//! \brief Shortcut to add a KEYVAL integer entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input semver 'semver'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status dc_add_keyval_integer (dc_t *parent, const char *name, int64_t def,
                                         const char *doc, struct semantic_version *semver);

//! \brief Shortcut to add a KEYVAL float entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input semver 'semver'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status dc_add_keyval_float (dc_t *parent, const char *name, double def,
                                       const char *doc, struct semantic_version *semver);

//! \brief Shortcut to add a KEYVAL boolean entry to a parent context.
//!
//! Instead of beginning a context on a parent, setting the required name, type,
//! default, documentation fields, this function wraps all that logic into one
//! simple function call. If you require special restrictions and additional defaults,
//! you will need to go the long route through beginning and finalizing a context.
//!
//! \return DISIR_STATUS_OK if the parent accepted the input keyval with 'name', at
//!     the input semver 'semver'.
//! \return DISIR_STATUS_INVALID_ARGUMENT if either 'name', 'def' or 'doc' are NULL.
//! \return DISIR_STATUS_WRONG_CONTEXT if input 'parent' is of wrong context type.
//!
enum disir_status dc_add_keyval_boolean (dc_t *parent, const char *name, uint8_t def,
                                         const char *doc, struct semantic_version *semver);


//
// Restriction related context API
//

//! Return the disir_restriction_type enum type pointed to bytes
//! the passed disir_context_restriction context.
//! If any other context than disir_context_restriction is supplied,
//! an DISIR_STATUS_WRONG_CONTEXT is returned.
enum disir_status dc_restriction_get_type (dc_t *restriction, enum disir_restriction *type);


//
// Config related context API
//

//! \brief Retrieve the context associated with an already constructed disir_config.
//!
//! This context may be used to manipulate or query the config object.
//!
//! \return context of type DISIR_CONTEXT_CONFIG
//!
dc_t * dc_config_getcontext (struct disir_config *config);

//! \brief Get the version number of this config.
//!
//! \param[in] config Input schema to retrieve semver for
//! \param[out] semver Output structure populated with the semver of config.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if config or semver are NULL
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_config_get_version (struct disir_config *config, struct semantic_version *semver);


//! \brief Begin construction of a CONFIG context based on the passed schema.
//!
//! \param[in] schema Input schema that this CONFIG object shall represent.
//! \param[out] config Output CONFIG context object.
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dc_config_begin (struct disir_schema *schema, dc_t **config);

//! \brief Finalize the construction of a DISIR_CONTEXT_CONFIG
//!
//! \param[in,out] context A CONFIG context to finalize. Will be sat to NULL on success.
//! \param[out] config The CONFIG object to populated on success.
//!
//! \return DISIRSTATUS_WRONG_CONTEXT if input context is not of type CONFIG.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status dc_config_finalize (dc_t **context, struct disir_config **config);

//
// Schema related context API
//

//! Retrieve the context associated with an already constructed disir_schema.
//! This context may be used to manipulate or query the schema object.
dc_t * dc_schema_getcontext (struct disir_schema *schema);

//! \brief Get the version number of this schema.
//!
//! \param[in] schema Input schema to retrieve semver for
//! \param[out] semver Output structure populated with the semver of schema.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if schema or semver are NULL
//! \return DISIR_STATUS_OK on success.
//!
enum disir_status
dc_schema_get_version (struct disir_schema *schema, struct semantic_version *semver);

//! Construct the DISIR_CONTEXT_SCHEMA
enum disir_status dc_schema_begin (dc_t **schema);

//! Finalize the construction of a DISIR_CONTEXT_SCHEMA, returning
//! an allocated disir_schema object in the output parameter.
//! If any unfinalized descendant contexts exists,
//! DISIR_STATUS_CONTEXT_IN_WRONG_STATE will be returned.
//! If the context supplied is not of type DISIR_CONTEXT_SCHEMA,
//! status DISIR_STATUS_WRONG_CONTEXT will be returned.
//! On success, DISIR_STATUS_OK is returned.
enum disir_status dc_schema_finalize (dc_t **context, struct disir_schema **schema);

#endif // _LIBDISIR_CONTEXT_H

