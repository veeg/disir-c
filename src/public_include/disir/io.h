#ifndef _LIBDISIR_IO_H
#define _LIBDISIR_IO_H

//! \brief Function signature for inputting disir_config from external source.
//!
//! \param[in] id String identifier for the config file to read.
//! \param[in] schema Optional schema to validate config against. If not provided, function
//!     is responsible to locate and create the schema associated with id.
//! \param[out] config The fully populated config object
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*config_read) (const char *id, struct disir_schema *schema,
                                          struct disir_config **config);

//! \brief Function signature for outputting disir_config to external source.
//!
//! \param[in] id String identifier for the config file to write.
//! \parm[in] config The config object to persist to 'id' locatiton.
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*config_write) (const char *id, struct disir_config *config);

//! \brief Function signature for inputting disir_schema from external source.
//!
//! \param[in] id String identifier for the schema file to read.
//! \param[out] schema Ouput schema to populate read from the 'id' source.
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*schema_read) (const char *id, struct disir_schema **schema);

//! \brief Function signature for outputting disir_schema to external source.
//!
//! \param[in] id String identifier for the schema file to write.
//! \param[int] schema The schema object to presist to 'id' location.
//!
//! \return DISIR_STATUS_OK on success
//!
typedef enum disir_status (*schema_write) (const char *id, struct disir_schema *schema);

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

//! \brief Input a config object with of I/O plugin 'type', identified by 'id'
//!
//! Read a config object identified by 'id' with I/O plugin 'type', registered in 'disir'.
//! The schema that describes the config may be NULL, which requires the I/O plugin
//! to also locate and read in the associated schema.
//!
//! \param[in] disir Library instance
//! \param[in] id Identifier for the config source to read.
//! \param[in] type Kind of I/O plugin to use on 'id' to populate 'config'
//! \param[in] schema Optional schema that describes the config to parse. If NULL,
//!     I/O plugin must locate a schema instead.
//! \param[out] config Object to populate with the state read from 'id'
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
disir_config_input (struct disir *disir, const char *id, const char *type,
                    struct disir_schema *schema, struct disir_config **config);


//! \brief Input a schema object with I/O plugin 'type', identified by 'id'
//!
//! Read a schema object identified by 'id' with I/O plugin 'type', registed in 'disir'.
//!
//! \param[in] disir Library instance
//! \param[in] id Identifier for the schema source to read.
//! \param[in] type Kind of I/O plugin to use on 'id' to populate 'schema'
//! \param[out] schema Object to populate with the state read from 'id'
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
disir_schema_input (struct disir *disir, const char *id,  const char *type,
                    struct disir_schema **schema);

//! \brief Output the config object to a register type output plugin
//!
//! \param disir Instance holding libdisir state
//! \param id Identifier of the schema to external source.
//! \param type Output type. Must be previously registered as so with disir.
//! \param config The config object to output to the external type format.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input arguments are
//!     NULL or the type cannot be found in the instance.
//! \return status of the output plugin
//!
enum disir_status
disir_config_output (struct disir *disir, const char *id, const char *type,
                     struct disir_config *config);

//! \brief Output the schema object to a register type output plugin
//!
//! \param disir Instance holding libdisir state
//! \param id Identifier of the schema to external source.
//! \param type Output type. Must be previously registered as so with disir.
//! \param schema The schema object to output to the external type format.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input arguments are
//!     NULL or the type cannot be found in the instance.
//! \return status of the output plugin
//!
enum disir_status
disir_schema_output (struct disir *disir, const char *id, const char *type,
                     struct disir_schema *schema);


#endif // _LIBDISIR_IO_H

