#ifndef _LIBDISIR_RESTRICTION_H
#define _LIBDISIR_RESTRICTION_H

#include <disir/context.h>


struct disir_restriction
{
    //! Context object of this restriction.
    struct disir_context        *re_context;

    //! Version this restriction entry was introduced.
    struct semantic_version     re_introduced;
    //! Version this restriction entry was deprecated.
    struct semantic_version     re_deprecated;

    //! Documentation for this restriction.
    struct disir_documentation  *re_documentation_queue;

    //! The type of restriction this entry represents
    enum disir_restriction_type re_type;
    char                        *re_value_string;
    double                      re_value_numeric;
    double                      re_value_min;
    double                      re_value_max;


    //! Double linked-list pointers.
    struct disir_restriction    *next;
    struct disir_restriction    *prev;
};


//! Only called from dc_begin
enum disir_status dx_restriction_begin (struct disir_context *parent,
                                        struct disir_context **restriction);

//! Only called from dc_finalize
enum disir_status dx_restriction_finalize (struct disir_context *restriction);

//! Allocate and initialize a new disir_restriction structure
struct disir_restriction * dx_restriction_create (struct disir_context *parent);

//! Free all resources belonging to disir_restriction structure (including itself)
enum disir_status dx_restriction_destroy (struct disir_restriction **restriction);

//! \brief Check KEYVAL context value type if within restriction bounds.
//!
//! \return DISIR_STATUS_OK if root context is not CONFIG.
//! \return DISIR_STATUS_INTERNAL_ERROR if context' value type is not INTEGER or FLOAT
//! \return DISIR_STATUS_RESTRICTION_VIOLATED if non of the exclusive restrictions
//!         applied to VALUE_RANGE or VALUE_NUMERIC are fulfilled.
//! \return DISIR_STATUS_OK if restrictions are fulfilled (if any).
//!
enum disir_status dx_restriction_exclusive_value_check (struct disir_context *context,
                                                        int64_t integer_value,
                                                        double float_value);


#endif // _LIBDISIR_RESTRICTION_H

