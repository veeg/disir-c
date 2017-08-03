#ifndef _LIBDISIRCLI_COMMAND_IMPORT_H
#define _LIBDISIRCLI_COMMAND_IMPORT_H

#include <set>

#include <disir/cli/cli.h>
#include <disir/cli/command.h>
#include <disir/archive.h>

namespace disir
{
    struct archive_entry_info
    {
        std::string ai_entry_id;
        std::string ai_group_id;
        std::string ai_info;
        std::string ai_version;
        enum disir_status ai_status;
    };

    using error_permutation = std::tuple<enum disir_import_option, enum disir_status,
                                         enum disir_status>;

    class CommandImport : public Command
    {
    private:
        // Mapping import option (0) to archive entry status (1)
        // and status returns from disir_archive_entry_resolve (2)
        std::map<error_permutation, std::string> import_option_error = {
            {std::make_tuple (DISIR_IMPORT_UPDATE, DISIR_STATUS_CONFLICT, DISIR_STATUS_NO_CAN_DO),
            "archive entry cannot be updated to a newer version"},
            {std::make_tuple (DISIR_IMPORT_UPDATE, DISIR_STATUS_CONFLICT,
                              DISIR_STATUS_RESTRICTION_VIOLATED),
            "elements in archive entry violates restrictions introduced on a higher version"},
            {std::make_tuple (DISIR_IMPORT_UPDATE, DISIR_STATUS_OK, DISIR_STATUS_NO_CAN_DO),
            "archive entry cannot be updated to a newer version"},
            {std::make_tuple (DISIR_IMPORT_UPDATE_WITH_DISCARD, DISIR_STATUS_CONFLICT,
                              DISIR_STATUS_NO_CAN_DO),
            "archive entry cannot be updated to a newer version"},
            {std::make_tuple (DISIR_IMPORT_DO, DISIR_STATUS_CONFLICT, DISIR_STATUS_NO_CAN_DO),
            "equivalent entry already exists on system, only option is force"},
            {std::make_tuple (DISIR_IMPORT_DO, DISIR_STATUS_CONFLICTING_SEMVER,
                              DISIR_STATUS_NO_CAN_DO),
            "archive entry version and its equivalent on the system differ, only option is update"},
            {std::make_tuple (DISIR_IMPORT_UPDATE_WITH_DISCARD, DISIR_STATUS_CONFLICTING_SEMVER,
                              DISIR_STATUS_NO_CAN_DO),
            "archive entry version and its equivalent on the system differ, only option is update"},
        };

    public:
        CommandImport (void);

        //! Handle command implementation
        virtual int handle_command (std::vector<std::string> &args);
    private:
        //! Import all entries with one option
        int import_all_option (struct disir_import *import, int entries,
                               enum disir_import_option option);

        //! Import entries one at a time.
        int archive_import_interactive (struct disir_import *import, int entries);

        //! Print a single entry - either with or withouth import status
        void print_archive_entries (std::list<struct archive_entry_info>& info);

        //! Interactive option reader
        int read_option (enum disir_import_option& option);

        enum disir_import_option prompt_finalize_yes_no_from_user ();

        void read_archive_entry (struct disir_import *import, int entry,
                                 struct archive_entry_info& entry_info);

        //! Get at most length characters from stding
        int read_stdin (char *opt, unsigned int length);

        //! Populate version with system entry version
        //! version is not populated id config does not exist
        int read_config_version (std::string& group_id, std::string& entry_id,
                                 std::string& version);

        //! Print a concated list of m_entries
        void print_import_commit_effects ();

        //! Format a string denoting the result of an import given an option
        void entry_import_result_format (struct archive_entry_info& entry,
                                         const char *system_entry_version,
                                         enum disir_import_option opt);
    private:
        std::map<std::string, std::string> m_entries;
    };
}

#endif // _LIBDISIRCLI_COMMAND_GENERATE_H

