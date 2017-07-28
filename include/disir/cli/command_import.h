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
        std::string ai_status;

    };

    class CommandImport : public Command
    {
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
        void print_archive_entries (std::list<struct archive_entry_info>& info, bool inspect);

        //! Interactive option reader
        int read_option (enum disir_import_option& option);

        void read_archive_entry (struct disir_import *import, int entry,
                                 std::list<struct archive_entry_info>& info);
    };
}

#endif // _LIBDISIRCLI_COMMAND_GENERATE_H

