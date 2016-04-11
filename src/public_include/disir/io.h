#ifndef _LIBDISIR_IO_H
#define _LIBDISIR_IO_H

//! Function signature for inputting disir_config from external source.
typedef enum disir_status (*config_read) (const char *id, struct disir_config **);
//! Function signature for outputting disir_config to external source.
typedef enum disir_status (*config_write) (const char *id, struct disir_config *);
//! Function signature for inputting disir_schema from external source.
typedef enum disir_status (*schema_read) (const char *id, struct disir_schema **);
//! Function signature for outputting disir_schema to external source.
typedef enum disir_status (*schema_write) (const char *id, struct disir_schema *);

//! Maximum number of bytes the 'type' parameter can identify an I/O type.
#define DISIR_IO_TYPE_MAXLENGTH             32
//! Maximum number of bytes the 'description' parameter can describe an I/O type.
#define DISIR_IO_DESCRIPTION_MAXLENGTH      255


//! \brief Register a read input plugin with the libdisir instance
//!
//! Register two read input methods to populate config and schema objects
//! identifed by a specific type.
//!
//! \param disir Instance to register input type with.
//! \param type Identifier of the input type. Must be within DISIR_IO_TYPE_MAXLENGTH bytes long.
//! \param description Describes the type format it reads from.
//! \param config A config_read function signature callback.
//! \param schema a schema_read function signature callback.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input parameters are NULL
//! \return DISIR_STATUS_NO_MEMORY if memory allocation failed.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
disir_register_input (struct disir *disir, const char *type, const char *description,
                      config_read config, schema_read schema);

//! \brief Register a write output plugin with the libdisir instance
//!
//! Register two write output methods to output config and schema objects
//! identifed by a specific type to external format.
//!
//! \param disir Instance to register output type with.
//! \param type Identifier of the output type. Must be within DISIR_IO_TYPE_MAXLENGTH bytes long.
//! \param description Describes the type format it write to.
//! \param config A config_write function signature callback.
//! \param schema a schema_write function signature callback.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input parameters are NULL
//! \return DISIR_STATUS_NO_MEMORY if memory allocation failed.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
disir_register_output (struct disir *disir, const char *type, const char *description,
                      config_write config, schema_write schema);

//! TODO: Doc
enum disir_status
disir_config_input (struct disir *disir, const char *type,
                   struct disir_config **config, const char *id);


//! TODO: Doc
enum disir_status
disir_schema_input (struct disir *disir, const char *type,
                   struct disir_schema **schema, const char *id);

//! \brief Output the config object to a register type output plugin
//!
//! \param disir Instance holding libdisir state
//! \param type Output type. Must be previously registered as so with disir.
//! \param config The config object to output to the external type format.
//! \param id Identifier of the schema to external source.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input arguments are
//!     NULL or the type cannot be found in the instance.
//! \return status of the output plugin
//!
enum disir_status
disir_config_output (struct disir *disir, const char *type,
                   struct disir_config *config, const char *id);

//! \brief Output the schema object to a register type output plugin
//!
//! \param disir Instance holding libdisir state
//! \param type Output type. Must be previously registered as so with disir.
//! \param schema The schema object to output to the external type format.
//! \param id Identifier of the schema to external source.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input arguments are
//!     NULL or the type cannot be found in the instance.
//! \return status of the output plugin
//!
enum disir_status
disir_schema_output (struct disir *disir, const char *type,
                   struct disir_schema *schema, const char *id);


#endif // _LIBDISIR_IO_H

