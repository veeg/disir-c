
//! Forward declare Disir Element Storage structure.
//! This is a private structure, even to the internals of Disir.
struct disir_element_storage;

//! \brief Allocate a new instance of the Disir Element Storage
//!
//! \return Pointer to the newly allocated instance. NULL if the allocation failed.
struct disir_element_storage *
dx_element_storage_create (void);

//! \brief Destroy a previously allocated instance of Disir Element Storage
//!
//! \param[in,out] storage Double-pointer to the allocated instance that shall be destroyed.
//!                 The pointer is set to NULL when the element storage is free'd.
//! \return DISIR_STATUS_OK when the storage was successfully destroyed.
//!
enum disir_status
dx_element_storage_destroy (struct disir_element_storage **storage);

enum disir_status
dx_element_storage_add (struct disir_element_storage *storage,
                        const char * const name,
                        struct disir_context *context);

enum disir_status
dx_element_storage_remove (struct disir_element_storage *storage,
                           const char * const name,
                           struct disir_context *context);

//! \brief Query the element storage for its elmenets matching name
//!
//! \param storage Element storage to query entries from
//! \param name String name of the element(s) to retrieve
//! \param[out] collection output of the matching entries, if any.
//! \return DISIR_STATUS_OK if a one or more entries were found.
//! \return DISIR_STATUS_EXHAUSTED if no entries were found.
enum disir_status
dx_element_storage_get (struct disir_element_storage *storage,
                        const char * const name,
                        struct disir_context_collection **collection);

//! XXX: Is this function needed?
enum disir_status
dx_element_storage_get_first_keyval (struct disir_element_storage *storage,
                                     const char *name,
                                     struct disir_context **context);

//! XXX: Is this function needed?
enum disir_status
dx_element_storage_get_first_section (struct disir_element_storage *storage,
                                      const char *name,
                                      struct disir_context **context);

