
#include <map>
#include <utility>
#include <string.h>

#include <disir/disir.h>


typedef enum disir_status (*output_mold)(struct disir_mold **);

#include "basic_keyval.cc"
#include "basic_section.cc"


// Forward declaration
enum disir_status
dio_test_config_read (struct disir_instance *disir, const char *id,
                      struct disir_mold *mold, struct disir_config **config);
enum disir_status
dio_test_config_list (struct disir_instance *disir, struct disir_collection **collection);
enum disir_status
dio_test_config_version (struct disir_instance *disir,
                         const char *id, struct semantic_version *semver);
enum disir_status
dio_test_mold_read (struct disir_instance *disir, const char *id, struct disir_mold **mold);
enum disir_status
dio_test_mold_list (struct disir_instance *disir, struct disir_collection **collection);

struct cmp_str
{
   bool operator()(char const *a, char const *b)
   {
      return strcmp(a, b) < 0;
   }
};

//! Internal static map to store test molds by id
static std::map<const char *, output_mold, cmp_str> molds = {
    std::make_pair ("basic_keyval", basic_keyval),
    std::make_pair ("basic_section", basic_section),
};

enum disir_status
dio_test_config_read (struct disir_instance *disir, const char *id,
                      struct disir_mold *mold, struct disir_config **config)
{
    enum disir_status status;
    output_mold func_mold;

    func_mold = molds[id];
    if (func_mold == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    status = func_mold (&mold);
    if (status != DISIR_STATUS_OK)
        return status;

    return disir_generate_config_from_mold (mold, NULL, config);
}

enum disir_status
dio_test_config_list (struct disir_instance *disir, struct disir_collection **collection)
{
    return dio_test_mold_list (disir, collection);
}

enum disir_status
dio_test_config_version (struct disir_instance *disir,
                         const char *id, struct semantic_version *semver)
{
    enum disir_status status;
    struct disir_mold *mold;
    output_mold func_mold;

    if (id == NULL || semver == NULL)
    {
        return DISIR_STATUS_INVALID_ARGUMENT;
    }

    func_mold = molds[id];
    if (func_mold == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    status = func_mold (&mold);
    if (status != DISIR_STATUS_OK)
        return status;

    return dc_mold_get_version (mold, semver);
}

enum disir_status
dio_test_mold_read (struct disir_instance *disir, const char *id, struct disir_mold **mold)
{
    enum disir_status status;
    output_mold func_mold;

    func_mold = molds[id];
    if (func_mold == NULL)
        return DISIR_STATUS_INVALID_ARGUMENT;

    status = func_mold (mold);
    if (status != DISIR_STATUS_OK)
        return status;

    return DISIR_STATUS_OK;;
}

enum disir_status
dio_test_mold_list (struct disir_instance *disir, struct disir_collection **collection)
{
    struct disir_collection *col;
    struct disir_context *context;

    col = dc_collection_create ();
    if (col == NULL)
    {
        return DISIR_STATUS_NO_MEMORY;
    }

    for (auto i = molds.begin(); i != molds.end(); ++i)
    {
        status = dc_free_text_create (i->first, &context);
        if (status != DISIR_STATUS_OK)
        {
            continue;
        }
        dc_collection_push_context (col, context);
        dc_putcontext (&context);
    }

    *collection = col;
    return DISIR_STATUS_OK;
}

extern "C" enum disir_status
dio_register_plugin (struct disir_instance *disir)
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

