#ifndef _LIBDISIR_PRIVATE_UPDATE_H
#define _LIBDISIR_PRIVATE_UPDATE_H

struct disir_update
{
    //! Target version to upgrade config to
    struct disir_version        up_target;
    //! Config to upgrade
    struct disir_config         *up_config_old;
    struct disir_config         *up_config_target;

    //! Current collection we're iterating
    //! TODO: When iterating nested sections of collections,
    //! we need to handle the nested level and store collections
    //! on a stack
    struct disir_collection       *up_collection;

    int                         up_updated;

    //! Conflict state - NULL when no conflict
    struct disir_keyval         *up_keyval;
    struct disir_context        *up_context_keyval;
    char                        *up_config_value;
    char                        *up_mold_value;
};

enum disir_status
dx_update_config_with_changes (struct disir_config **config, int discard_violations);

#endif // _LIBDISIR_PRIVATE_UPDATE_H

