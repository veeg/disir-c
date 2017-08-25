#ifndef _LIBDISIR_ARCHIVE_TEST_HELPER_H
#define _LIBDISIR_ARCHIVE_TEST_HELPER_H

#include <map>
#include <algorithm>

#include <gtest/gtest.h>
#include "test_helper.h"

extern "C" {
#include "log.h"
}

namespace testing
{
    class DisirTestArchive : public DisirTestWrapper
    {
    public:
        static void SetUpTestCase ();
        static void TearDownTestCase ();
        static void SetUpArchiveEnvironment (struct disir_instance *instance);
        static void GenerateAllEntries (struct disir_instance *instance,
                                        std::set<std::string>& entries,
                                        const char *group_name);
        void GenerateConfigOverrides();
        void TeardownConfigOverrides();


        void SetUp ()
        {
            DisirLogCurrentTestEnter ();
        }

        void TearDown ()
        {
            DisirLogCurrentTestExit ();
        }

    public:
        static struct disir_instance *instance_export;
        static struct disir_instance *instance_import;
        static struct disir_mold     *libdisir_mold_import;
        static struct disir_mold     *libdisir_mold_export;
        static struct disir_config   *libdisir_config_export;
        static struct disir_config   *libdisir_config_import;

        enum disir_status status;
        std::map<std::string, struct disir_config*> m_nondefault_configs;
    };
}

#endif // _LIBDISIR_TEST_HELPER_H

