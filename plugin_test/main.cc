
#include <map>
#include <utility>
#include <string.h>

#include <disir/disir.h>

#include "basic_keyval.cc"


// Forward declaration
enum disir_status
dio_test_config_read (struct disir *disir, const char *id,
                      struct disir_mold *mold, struct disir_config **config);
enum disir_status
dio_test_config_list (struct disir *disir, struct disir_collection **collection);
enum disir_status
dio_test_config_version (struct disir *disir, const char *id, struct semantic_version *semver);
enum disir_status
dio_test_mold_read (struct disir *disir, const char *id, struct disir_mold **mold);
enum disir_status
dio_test_mold_list (struct disir *disir, struct disir_collection **collection);


//! Internal static map to store test molds by id
static std::map<const char *, struct disir_mold *> molds
{
    basic_keyval (),
};

enum disir_status
dio_test_config_read (struct disir *disir, const char *id,
                      struct disir_mold *mold, struct disir_config **config)
{
    return disir_generate_config_from_mold (molds[id], NULL, config);
}

enum disir_status
dio_test_config_list (struct disir *disir, struct disir_collection **collection)
{
    return dio_test_mold_list (disir, collection);
}

enum disir_status
dio_test_config_version (struct disir *disir, const char *id, struct semantic_version *semver)
{
    struct disir_mold *mold;

    if (id == NULL || semver == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    mold = molds[id];
    if (mold == NULL)
    {
        // XXX STATUS: does not exist error
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    return dc_mold_get_version (mold, semver);
}

enum disir_status
dio_test_mold_read (struct disir *disir, const char *id, struct disir_mold **mold)
{
    *mold = molds[id];
    if (*mold == NULL)
    {
        // XXX STATUS: does not exist error
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    return DISIR_STATUS_OK;;
}

enum disir_status
dio_test_mold_list (struct disir *disir, struct disir_collection **collection)
{
    struct disir_collection *col;
    dc_t *context;

    col = dc_collection_create ();
    if (col == NULL)
    {
        return DISIR_STATUS_NO_MEMORY;
    }

    for (auto i = molds.begin(); i != molds.end(); ++i)
    {
        dc_free_text_create (i->first, &context);
        dc_collection_push_context (col, context);
        dc_putcontext (&context);
    }

    *collection = col;
    return DISIR_STATUS_OK;
}

extern "C" enum disir_status
dio_register_plugin (struct disir *disir)
{
    struct disir_input_plugin input;

    input.in_struct_size = sizeof (struct disir_input_plugin);
    input.in_config_read = dio_test_config_read;
    input.in_config_list = dio_test_config_list;

    input.in_config_version = dio_test_config_version;
    input.in_mold_read = dio_test_mold_read;
    input.in_mold_list = dio_test_mold_list;

    return disir_register_input (disir, "test", "libdisir test molds", &input);
}

