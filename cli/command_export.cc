// private
#include <disir/cli/command_export.h>

#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

//disir
#include <disir/disir.h>
#include <disir/fslib/util.h>
#include <disir/archive.h>
#include <disir/fslib/json.h>

using namespace disir;

CommandExport::CommandExport (void)
    : Command ("export"),
      m_tempdir("/tmp/disir/"),
      m_archive_name("/archive.disir")
{
    // TODO: Check m_tmpdir access rights
}

int
CommandExport::handle_command (std::vector<std::string> &args)
{
    std::stringstream group_description;
    args::ArgumentParser parser ("Export configuration entries to disir_archive");

    setup_parser (parser);
    parser.Prog ("disir export");

    args::HelpFlag help (parser, "help", "Display the list help menu and exit.",
                         args::Matcher{'h', "help"});
    group_description << "Specify the group to operate on. The loaded default is: "
                      << m_cli->group_id();
    args::ValueFlag<std::string> opt_group_id (parser, "NAME", group_description.str(),
                                               args::Matcher{"group"});

    args::ValueFlag<std::string> opt_dest (parser, "DEST",
                                           "Directory in which to place archive. Default is" \
                                           " cwd.",
                                           args::Matcher{'d', "dest"});

    args::ValueFlag<std::string> opt_source (parser, "SOURCE",
                                             "Filepath to existing archive.",
                                             args::Matcher {"source"});

    args::Flag opt_append (parser, "append",
                           "Append configuration entries to an existing archive",
                           args::Matcher{"append"});

    args::PositionalList<std::string> opt_entries (parser, "entry",
                                                   "A list of entries to file.");

    // TODO: Allow setting outputdir
    try
    {
        parser.ParseArgs (args);
    }
    catch (args::Help&)
    {
        std::cout << parser;
        return (0);
    }
    catch (args::ParseError& e)
    {
        std::cerr << "ParseError: " << e.what() << std::endl;
        std::cerr << "See '" << m_cli->m_program_name << " --help'" << std::endl;
        return (1);
    }
    catch (args::ValidationError& e)
    {
        std::cerr << "ValidationError: " << e.what() << std::endl;
        std::cerr << "See '" << m_cli->m_program_name << " --help'" << std::endl;
        return (1);
    }

    std::string group_id = m_cli->group_id();
    if (opt_group_id)
    {
        group_id = args::get (opt_group_id);
        m_cli->group_id (group_id);
    }

    std::set<std::string> entries_to_file;
    if (opt_entries)
    {
        for (const auto& entry : args::get (opt_entries))
        {
            entries_to_file.insert (entry);
        }
    }
    else
    {
        if (list_configs (entries_to_file))
        {
            return (1);
        }
    }

    if (opt_dest)
    {
        m_cli->verbose () << "Using user-provided destination directory" << std::endl;
        m_archive = args::get (opt_dest) + m_archive_name;
    }
    else
    {
        std::string cwd;
        if (current_working_directory (cwd) == false)
        {
            std::cout << "Unable to read current working directory - exiting...." << std::endl;
            return (1);
        }

        m_cli->verbose () << "Writing archive to current working directory: "
                          << cwd << std::endl;
        m_archive = cwd + m_archive_name;
    }

    if (opt_append)
    {
        if (!opt_source)
        {
            std::cerr << "Source archive path needed for append" << std::endl;
            return (1);
        }

        return populate_archive (args::get (opt_source).c_str(),
                                 args::get (opt_source).c_str(),
                                 group_id, entries_to_file);
    }

    return populate_archive (NULL, m_archive.c_str(), group_id, entries_to_file);
}

bool
CommandExport::current_working_directory (std::string& current)
{
    char buf[PATH_MAX];
    const char *cwd;

    cwd = getcwd (buf, PATH_MAX);
    if (cwd == NULL)
    {
        return false;
    }

    current = std::string(cwd);
    return true;
}

int
CommandExport::append_entries (struct disir_archive *archive,
                               std::string& group_id, std::set<std::string>& entries)
{
    for (const auto& entry : entries)
    {
        auto status = disir_archive_append_entry (m_cli->disir(), archive,
                                                  group_id.c_str(), entry.c_str());
        if (status != DISIR_STATUS_OK && status != DISIR_STATUS_NOT_EXIST)
        {
            std::cerr << "Failed to insert " << entry << " into archive: "
                      << disir_status_string (status) << std::endl;
            return(1);
        }
        else if (status == DISIR_STATUS_NOT_EXIST)
        {
            std::cerr << "No entry (" << entry << ") in group '" << group_id << "'\n";
            return(1);
        }
    }
    return(0);
}

int
CommandExport::populate_archive (const char *path, const char *dest_path,
                                 std::string& group_id,
                                 std::set<std::string>& entries)
{
    struct disir_archive *archive;
    int ret = 0;

    auto status = disir_archive_export_begin (m_cli->disir (), path, &archive);
    if (status != DISIR_STATUS_OK)
    {
        std::cout << "Unable to begin archive: " << disir_status_string (status) << std::endl;
        return(1);
    }

    ret = append_entries (archive, group_id, entries);
    if (ret)
    {
        // discard archive
        dest_path = NULL;
        m_cli->verbose() << "Discarding archive" << std::endl;
    }

    status = disir_archive_finalize (m_cli->disir(), dest_path, &archive);
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << disir_status_string (status) << ":" << std::endl;
        auto error = disir_error (m_cli->disir ());
        if (error)
            std::cout << error << std::endl;

        return(1);
    }

    if (ret == 0)
    {
        std::cout << "Config entries successfully filed:" << std::endl;
        for (const auto& entry : entries)
        {
            std::cout << "    " << entry << std::endl;
        }
    }

    return ret;
}

