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
//! or DISIR_CONTEXT_TEMPLATE. The root context will determine the
//! effect and allowed operations on child contexts.
struct disir_context;

//! Define a shortcut for struct disir_context, since it
//! is a often used construct.
typedef struct disir_context dc_t;

//! Different types of disir_contexts that are available.
enum disir_context_type
{
    DISIR_CONTEXT_CONFIG = 1,
    DISIR_CONTEXT_SCHEMA,
    DISIR_CONTEXT_TEMPLATE,
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
//!     * TEMPLATE
//! If an unsupported context is provided, DISIR_STATUS_WRONG_CONTEXT
//! is returned. If a documention entry already exists for this element,
//! DISIR_STATUS_EXISTS will be returned.
//! On success, DISIR_STATUS_OK is returned.
enum disir_status dc_add_documentation (dc_t *context, char *doc, int32_t doc_size);


//
// Add related context API
//

//! Add a name to an entry.
//! One can only add a name to a context who is in construction mode.
//! This is required on supported contexts DISIR_CONTEXT_KEYVAL and
//! DISIR_CONTEXT_GROUP.
//! If an unsupported context is provided DISIR_STATUS_WRONG_CONTEXT
//! is returned. If the entry already has a name, then DISIR_STATUS_EXISTS
//! will be returned.
//! On success, DISIR_STATUS_OK is returned.
enum disir_status dc_add_name (dc_t *context, char *name, int32_t name_size);

//! Add an introduced semantic version number to an entry.
//! One can only add an introduced semver to a context who is in construction mode.
//! Add a deprecrated semantic version number to an entry.
//! Depending on the context, checks for compatabilit with existing
//! elements in the parent may result in an errornous condition.
//! DISIR_STATUS_EXISTS is returned if an introduced entry already exists.
//! DISIR_STATUS_OK is returned on success.
enum disir_status dc_add_introduced(dc_t *context, struct semantic_version semver);
enum disir_status dc_add_deprecrated(dc_t *context, struct semantic_version smever);


//! One may only add types to a context in construction mode.
//! Only applicable to the DISIR_CONTEXT_SCHEMA or DISIR_CONTEXT_TEMPLATE
//! root contexts.
//! Only applicable to the DISIR_CONTEXT_KEYVAL parent context.
//! A keyval MUST have a valid type at all times.
enum disir_status dc_add_type (dc_t *context, enum disir_type type);

//! Add a default value to an entry.
// XXX: Should this be revised? Removed? Renamed?
enum disir_status dc_add_default (dc_t *context, void *value, int32_t value_size);


//! Add a string as value to the context.
//! Not applicable to every context.
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


//! Query the context for the first child with the name child_name.
dc_t *dc_get_child (dc_t *parent, const char *child_name, int32_t child_name_size);

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

//! Retrieve the context associated with an already constructed disir_config.
//! This context may be used to manipulate or query the config object.
dc_t * dc_config_getcontext (struct disir_config *config);

//! Create a DISIR_CONTEXT_CONFIG, which is a root context
//! used to create a disir_config object.
//! The returned context will reject any attempts to
//! add entries to it until a schema is attached.
enum disir_status dc_config_begin (dc_t **config);

//! Create a DISIR_CONTEXT_CONFIG, which is a root context
//! used to create a disir_config object.
//! It will locate the corresponding disir_schema in the default
//! location. If no such schema can be located or constructed, NULL is returned.
//! The schema is attempted located based on the config identifier
//! supplied.
enum disir_status dc_config_begin_default_schema (const char *config_identifier,
                                                  int32_t config_identifier_size,
                                                  dc_t **config);

//! \see dc_config_begin
//! Construct the DISIR_CONTEXT_CONFIG
//! The disir_schema object is provided.
//! The only scenarios where this may return NULL is in case of no
//! more memory or the supplied schema is in invalid state.
enum disir_status dc_config_begin_supplied_schema (struct disir_schema *schema, dc_t **config);

//! Finalize the construction of a DISIR_CONTEXT_CONFIG, returning
//! an allocated disir_config object instead.
//! If any unfinalized descendant contexts exists, DISIR_STATUS_CONTEXT_IN_WRONG_STATE
//! will be returned.
//! If the context supplied is not of type DISIR_CONTEXT_CONFIG,
//! status DISIR_STATUS_WRONG_CONTEXT will be returned.
//! On success, DISIR_STATUS_OK is returned.
enum disir_status dc_config_finalize (dc_t **context, struct disir_config **config);

//! Associate a schema object with the supplied config.
//! If the config context already has an attached schema,
//! the call fails with DISIR_STATUS_CONTEXT_IN_WRONG_STATE
enum disir_status dc_config_attach_schema (dc_t *config, struct disir_schema *schema);

//
// Schema related context API
//

//! Retrieve the context associated with an already constructed disir_schema.
//! This context may be used to manipulate or query the schema object.
dc_t * dc_schema_getcontext (struct disir_schema *schema);

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

//! Associate a disir_template with a schema context.
//! The template is attached to the inner disir_schema
//! associated with the passed schema context.
//! That is, when the context is finalized, the template
//! is still associated with the underlying disir_schema.
enum disir_status dc_schema_attach_template (dc_t *schema, struct disir_template *templ);

//
// Template related context API
//

//! Retrieve the context associated with an already constructed disir_template.
//! This context may be used to manipulate or query the schema object.
dc_t * dc_template_getcontext (struct disir_template *templ);

//! Construct the DISIR_CONTEXT_TEMPLATE
enum disir_status dc_template_begin (dc_t **templ);

//! Finalize the construction of a DISIR_CONTEXT_TEMPLATE, returning
//! an allocated disir_template object in the output parameter.
//! If any unfinalized descendant contexts exists,
//! DISIR_STATUS_CONTEXT_IN_WRONG_STATE will be returned.
//! If the context supplied is not of type DISIR_CONTEXT_TEMPLATE,
//! status DISIR_STATUS_WRONG_CONTEXT will be returned.
//! On success, DISIR_STATUS_OK is returned.
enum disir_status dc_template_finalize (dc_t **context, struct disir_template **templ);


#endif // _LIBDISIR_CONTEXT_H

