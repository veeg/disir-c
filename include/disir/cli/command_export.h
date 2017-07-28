#ifndef _LIBDISIRCLI_COMMAND_EXPORT_H
#define _LIBDISIRCLI_COMMAND_EXPORT_H

#include <string>

#include <disir/cli/cli.h>
#include <disir/cli/command.h>
#include <disir/archive.h>

namespace disir
{
    class CommandExport : public Command
    {
    public:
        CommandExport (void);

        //! Handle command implementation
        virtual int handle_command (std::vector<std::string> &args);

    private:
        // dir in which to create archive
        std::string m_archive = "";
        // working directory
        std::string m_tempdir = "";
        // name of archive
        std::string m_archive_name = "";

        // Creates and export archive of config entries
        int populate_archive (const char *path, const char *dest_path,
                              std::string& groud_id,
                              std::set<std::string>& entries);

        // Append entries to archive
        int append_entries (struct disir_archive *archive,
                            std::string& group_id, std::set<std::string>& entries);

        // read cwd
        bool current_working_directory (std::string& current);
    };
}

#endif // _LIBDISIRCLI_COMMAND_EXPORT_H
