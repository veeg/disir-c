#ifndef _LIBDISIR_IO_H
#define _LIBDISIR_IO_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus


//! \brief Function signature for inputting disir_config from external source.
//!
//! \param[in] id String identifier for the config file to read.
//! \param[in] mold Optional mold to validate config against. If not provided, function
//!     is responsible to locate and create the mold associated with id.
//! \param[out] config The fully populated config object
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*config_read) (struct disir *disir,
                                          const char *id,
                                          struct disir_mold *mold,
                                          struct disir_config **config);

//! \brief Function signature for outputting disir_config to external source.
//!
//! \param[in] id String identifier for the config file to write.
//! \param[in] config The config object to persist to 'id' locatiton.
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*config_write) (struct disir *disir,
                                           const char *id,
                                           struct disir_config *config);

//! \brief Retrieve all configs available
//! TODO: Docs
//! XXX: implement as output list?
typedef enum disir_status (*config_list) (struct disir *disir,
                                          struct disir_collection **collection);

//! \brief Retrieve the mold version from this configuration file
//!
//! \param[in] id String identifier for the config file to query for version.
//! \param[out] semver Semantic version number of this config file.
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*config_version) (struct disir *disir,
                                             const char *id,
                                             struct semantic_version *semver);

//! \brief Query if the config with passed id exists
typedef enum disir_status (*config_query) (struct disir *disir,
                                           const char *id);

//! \brief Function signature for inputting disir_mold from external source.
//!
//! \param[in] id String identifier for the mold file to read.
//! \param[out] mold Ouput mold to populate read from the 'id' source.
//!
//! \return DISIR_STATUS_OK on success.
//!
typedef enum disir_status (*mold_read) (struct disir *disir,
                                        const char *id,
                                        struct disir_mold **mold);

//! \brief Function signature for outputting disir_mold to external source.
//!
//! \param[in] id String identifier for the mold file to write.
//! \param[in] mold The mold object to presist to 'id' location.
//!
//! \return DISIR_STATUS_OK on success
//!
typedef enum disir_status (*mold_write) (struct disir *disir,
                                         const char *id,
                                         struct disir_mold *mold);

//! TODO: Docs
//! XXX: implement as output list?
typedef enum disir_status (*mold_list) (struct disir *disir,
                                        struct disir_collection **collection);

//! \brief Query if the mold with passed id exists
//! TODO: docs
typedef enum disir_status (*mold_query) (struct disir *disir,
                                         const char *id);

//! Maximum number of bytes the 'type' parameter can identify an I/O type.
#define DISIR_IO_TYPE_MAXLENGTH             32
//! Maximum number of bytes the 'description' parameter can describe an I/O type.
#define DISIR_IO_DESCRIPTION_MAXLENGTH      255


//! \brief Structure containing callbacks to register a disir input plugin.
struct disir_input_plugin
{
    //! Structure size in bytes of the disir_input_plugin structure.
    uint64_t        in_struct_size;

    //! Callback to read configuration from external source.
    config_read     in_config_read;
    //! Callback to list available configurations from external source.
    //! Output will contain only the generated configurations found
    //! in the mold list.
    config_list     in_config_list;
    //! Callback to query mold version of a given configuration.
    config_version  in_config_version;

    //! Callback to read mold from external source.
    mold_read       in_mold_read;
    //! Callback to list available molds from external source.
    mold_list       in_mold_list;
};

//! \brief Structure containing callbacks to register a disir output plugin.
struct disir_output_plugin
{
    //! Structure size in bytes of the disir_output_plugin structure.
    uint64_t        out_struct_size;

    //! Callback to output configration to external source.
    config_write    out_config_write;

    //! Callback to output mold to external source.
    mold_write      out_mold_write;
};


//! \brief Register a read input plugin with the libdisir instance
//!
//! Register two read input methods to populate config and mold objects
//! identifed by a specific type.
//!
//! \param disir Instance to register input type with.
//! \param type Identifier of the input type. Must be within DISIR_IO_TYPE_MAXLENGTH bytes long.
//! \param description Describes the type format it reads from.
//! \param plugin Structure containing all input callbacks implemented
//!     in the input plugin.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input parameters are NULL
//! \return DISIR_STATUS_NO_MEMORY if memory allocation failed.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
disir_register_input (struct disir *disir, const char *type, const char *description,
                      struct disir_input_plugin *plugin);

//! \brief Register a write output plugin with the libdisir instance
//!
//! Register two write output methods to output config and mold objects
//! identifed by a specific type to external format.
//!
//! \param disir Instance to register output type with.
//! \param type Identifier of the output type. Must be within DISIR_IO_TYPE_MAXLENGTH bytes long.
//! \param description Describes the type format it write to.
//! \param plugin Structure containing all output callbacks implemented in this output plugin.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input parameters are NULL
//! \return DISIR_STATUS_NO_MEMORY if memory allocation failed.
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
disir_register_output (struct disir *disir, const char *type, const char *description,
                       struct disir_output_plugin *plugin);

//! \brief Input a config object with of I/O plugin 'type', identified by 'id'
//!
//! Read a config object identified by 'id' with I/O plugin 'type', registered in 'disir'.
//! The mold that describes the config may be NULL, which requires the I/O plugin
//! to also locate and read in the associated mold.
//!
//! \param[in] disir Library instance
//! \param[in] type Kind of I/O plugin to use on 'id' to populate 'config'
//! \param[in] id Identifier for the config source to read.
//! \param[in] mold Optional mold that describes the config to parse. If NULL,
//!     I/O plugin must locate a mold instead.
//! \param[out] config Object to populate with the state read from 'id'
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
disir_config_input (struct disir *disir, const char *type, const char *id,
                    struct disir_mold *mold, struct disir_config **config);


//! \brief Input a mold object with I/O plugin 'type', identified by 'id'
//!
//! Read a mold object identified by 'id' with I/O plugin 'type', registed in 'disir'.
//!
//! \param[in] disir Library instance
//! \param[in] type Kind of I/O plugin to use on 'id' to populate 'mold'
//! \param[in] id Identifier for the mold source to read.
//! \param[out] mold Object to populate with the state read from 'id'
//!
//! \return DISIR_STATUS_OK on success
//!
enum disir_status
disir_mold_input (struct disir *disir, const char *type, const char *id,
                    struct disir_mold **mold);

//! \brief Output the config object to a register type output plugin
//!
//! \param disir Instance holding libdisir state
//! \param type Output type. Must be previously registered as so with disir.
//! \param id Identifier of the mold to external source.
//! \param config The config object to output to the external type format.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input arguments are
//!     NULL or the type cannot be found in the instance.
//! \return status of the output plugin
//!
enum disir_status
disir_config_output (struct disir *disir, const char *type, const char *id,
                     struct disir_config *config);

//! \brief Output the mold object to a register type output plugin
//!
//! \param disir Instance holding libdisir state
//! \param type Output type. Must be previously registered as so with disir.
//! \param id Identifier of the mold to external source.
//! \param mold The mold object to output to the external type format.
//!
//! \return DISIR_STATUS_INVALID_ARGUMENT if either of the input arguments are
//!     NULL or the type cannot be found in the instance.
//! \return status of the output plugin
//!
enum disir_status
disir_mold_output (struct disir *disir, const char *type, const char *id,
                     struct disir_mold *mold);

//! \brief List the available configuration files from this input plugin
//!
//! \param type Input type. Must be previously registed with disir
enum disir_status
disir_config_list (struct disir *disir, const char *type,
                   struct disir_collection **collection);


#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_IO_H

