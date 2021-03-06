#ifndef _LIBDISIR_TEST_H
#define _LIBDISIR_TEST_H

#ifdef __cplusplus
extern "C"{
#endif // _cplusplus

#include <disir/disir.h>

DISIR_EXPORT
enum disir_status dio_test_config_read (struct disir_instance *instance,
                                        struct disir_register_plugin *plugin, const char *entry_id,
                                        struct disir_mold *mold, struct disir_config **config);

DISIR_EXPORT
enum disir_status dio_test_config_entries (struct disir_instance *instance,
                                           struct disir_register_plugin *plugin,
                                           struct disir_entry **entries);

DISIR_EXPORT
enum disir_status dio_test_config_query (struct disir_instance *instance,
                                         struct disir_register_plugin *plugin,
                                         const char *entry_id,
                                         struct disir_entry **entry);

DISIR_EXPORT
enum disir_status dio_test_mold_read (struct disir_instance *instance,
                                      struct disir_register_plugin *plugin,
                                      const char *entry_id,
                                      struct disir_mold **mold);

DISIR_EXPORT
enum disir_status dio_test_mold_entries (struct disir_instance *instance,
                                         struct disir_register_plugin *plugin,
                                         struct disir_entry **entries);

DISIR_EXPORT
enum disir_status dio_test_mold_query (struct disir_instance *instance,
                                       struct disir_register_plugin *plugin,
                                       const char *entry_id,
                                       struct disir_entry **entry);

#ifdef __cplusplus
}
#endif // _cplusplus

#endif // _LIBDISIR_TEST_H

