
//! Nested naming scheme
//! e.g. "section@2.keyval@3, which accesses the section element 'section', whose element
//! third element 'keyval'
//!
//! Normal procedure would simply be "section.section.keyval"
//!
//! Setters with invalid range (e.g., "keyval@3", where there only exists 1 entry already,
//! meaning the logical entry to set is either 1 (already exists, override) or 2 (add entry)


enum disir_status
disir_config_get_keyval_integer (struct disir_config *config, int64_t *value,
                                 const char *name, ...)
{
    // Loopup keyval (which can be recursive) from name.
}

enum disir_status
disir_config_set_keyval_integer (struct disir_config *config, int64_t value,
                                 const char *name, ...)
{
    // Loopup keyval (which can be recursive) from name.
}

enum disir_status
disir_config_get_keyval_float (struct disir_config *config, double value,
                               const char *name, ...)
{
    // Loopup keyval (which can be recursive) from name.
}


enum disir_status
disir_config_set_keyval_float (struct disir_config *config, double value,
                               const char *name, ...)
{
    // Loopup keyval (which can be recursive) from name.
}

//! Access both enum and string values from this.
enum disir_status
disir_config_get_keyval_string (struct disir_config *config, const char **value,
                                const char *name, ...)
{
    // Loopup keyval (which can be recursive) from name.
}

enum disir_status
disir_config_set_keyval_string (struct disir_config *config, const char *value,
                                const char *name, ...)
{
    // Loopup keyval (which can be recursive) from name.
}

