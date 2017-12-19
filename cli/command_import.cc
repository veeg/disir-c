#include <iostream>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <limits.h>
#include <list>

#include <disir/disir.h>
#include <disir/fslib/util.h>
#include <disir/archive.h>

#include <disir/cli/command_import.h>
#include <disir/cli/args.hxx>

using namespace disir;

CommandImport::CommandImport(void)
    : Command ("import")
{
}

int
CommandImport::handle_command (std::vector<std::string> &args)
{
    args::ArgumentParser parser ("Inspect or import entries from disir archive.");
    setup_parser (parser);
    parser.Prog ("disir import");

    args::HelpFlag help (parser, "help", "Display the generate help menu and exit.",
                         args::Matcher{'h', "help"});

    args::Group archive_required (parser, "Requred option:",
                                 args::Group::Validators::AtLeastOne);

    args::ValueFlag<std::string> opt_archive_path (archive_required, "ARCHIVE",
                                                   "Filepath to disir archive.",
                                                   args::Matcher{"archive"});

    args::Group import_opt_group (parser, "Import options:",
                                  args::Group::Validators::Xor);

    args::Group prompt_opt_group (parser, "At most one in combination with import option.",
                                  args::Group::Validators::AtMostOne);

    args::Flag opt_interactive (import_opt_group, "interactive",
                                "Import and resolve archive entries one by one.",
                                args::Matcher{"interactive", 'i'});

    args::Flag opt_force (import_opt_group, "force",
                          "Force import (importable) entries in archive - archive wins.",
                          args::Matcher{"force"});

    args::Flag opt_soft (import_opt_group, "soft",
                         "System + update entries that are not up-to-date.",
                         args::Matcher{"soft"});

    args::Flag opt_system (import_opt_group, "system",
                           "Import all entries which do not conflict with system - system wins.",
                           args::Matcher{"system"});

    args::Flag opt_inspect (import_opt_group, "inspect", "Print entries in archive.",
                            args::Matcher{"inspect"});

    args::Flag opt_accept (prompt_opt_group, "accept", "Accept changes in advance.",
                           args::Matcher{"accept"});

    args::Flag opt_dry_run (prompt_opt_group, "dry-run", "Print the effects of an import option.",
                            args::Matcher{"dry-run"});

    try
    {
        parser.ParseArgs (args);
    }
    catch (args::Help)
    {
        std::cout << parser;
        return (0);
    }
    catch (args::ParseError e)
    {
        std::cerr << "ParseError: " << e.what() << std::endl;
        std::cerr << "See '" << parser.Prog() << " --help'" << std::endl;
        return (1);
    }
    catch (args::ValidationError e)
    {
        std::cerr << "ValidationError: " << e.what() << std::endl;
        std::cerr << "See '" << parser.Prog() << " --help'" << std::endl;
        return (1);
    }

    if (opt_archive_path)
    {
        struct disir_import *import;
        enum disir_status status;
        struct import_report *report = NULL;
        enum disir_import_option option;
        std::string archive_path = args::get (opt_archive_path);
        std::list<struct archive_entry_info> info;
        struct archive_entry_info entry_info;
        int ret = 1;
        int entries;

        status = disir_archive_import (m_cli->disir(), archive_path.c_str(), &import, &entries);
        if (status != DISIR_STATUS_OK)
        {
            std::cerr << "Could not import archive (" << archive_path << "): "
                      << disir_error (m_cli->disir()) << std::endl;
            return (1);
        }

        if (opt_inspect)
        {
            std::cout << "There are " << entries << " entries in the archive:\n" << std::endl;
            for (auto i = 0; i < entries; i++)
            {
                read_archive_entry (import, i, entry_info);
                info.push_back (entry_info);
            }
            print_archive_entries (info);
        }
        else if (opt_interactive)
        {
            ret = archive_import_interactive (import, entries);
        }
        // Force import archive entries
        else if (opt_force)
        {
            ret = import_all_option (import, entries, DISIR_IMPORT_FORCE);
        }
        else if (opt_soft)
        {
            ret = import_all_option (import, entries, DISIR_IMPORT_UPDATE);
        }
        else if (opt_system)
        {
            ret = import_all_option (import, entries, DISIR_IMPORT_DO);
        }

        option = (!opt_dry_run && ret == 0) ? DISIR_IMPORT_DO : DISIR_IMPORT_DISCARD;

        if (ret == 0 && !opt_accept)
        {
            print_import_commit_effects ();
            if (!opt_dry_run)
            {
                option = prompt_finalize_yes_no_from_user ();
            }
        }

        status = disir_import_finalize (m_cli->disir(), option, &import, &report);
        if (status != DISIR_STATUS_OK)
        {
            if (disir_error (m_cli->disir()))
            {
                std::cout << disir_error (m_cli->disir()) << std::endl;
            }
            return(1);
        }
        if (report)
        {
            std::cout << "import summary: " << std::endl;
            for (auto i = 0; i < report->ir_entries; i++)
            {
                std::cout << "      " << report->ir_entry[i] << std::endl;
            }
            disir_import_report_destroy (&report);
        }
        // if import finalize fails to import an archive entry,
        // the only notification we get is that disir_error is populated
        if (disir_error (m_cli->disir()))
        {
            std::cerr << disir_error (m_cli->disir());
        }
    }
    else
    {
        std::cerr << "path to disir archive must be set '--archive'" << std::endl;
    }

    return 0;
}

enum disir_import_option
CommandImport::prompt_finalize_yes_no_from_user ()
{
    char opt[3];

    do
    {
        std::cout << "\ndo you wish to commit the above changes? (y)es or (n)o ? ";
        read_stdin (opt, 3);
        switch (opt[0])
        {
        case 'y':
            return DISIR_IMPORT_DO;
        case 'n':
            return DISIR_IMPORT_DISCARD;
        default:
            continue;
        }
    }
    while (1);

    return DISIR_IMPORT_DISCARD;
}

int
CommandImport::read_stdin (char *opt, unsigned int length)
{
    std::string input;
    std::getline (std::cin, input);

    if (input.length() > length)
    {
        return(1);
    }

    for (auto i = 0u; i < length || i < input.length(); i++)
    {
        opt[i] = input[i];
    }

    return 0;
}

int
CommandImport::read_option (enum disir_import_option& option)
{
    char opt;

    do
    {
        std::cout << "import entry [a,u,s,i,f,d,?]? ";

        if (read_stdin (&opt, 1))
        {
            opt = 'h';
        }

        switch (opt)
        {
            case 'a':
                return(1);
            case 'u':
                option = DISIR_IMPORT_UPDATE;
                break;
            case 'i':
                option = DISIR_IMPORT_DO;
                break;
            case 'f':
                option = DISIR_IMPORT_FORCE;
                break;
            case 's':
                option = DISIR_IMPORT_UPDATE_WITH_DISCARD;
                break;
            case 'd':
                option = DISIR_IMPORT_DISCARD;
                break;
            case 'h':
            case '?':
            default:
                std::cout << "a - abort\n";
                std::cout << "u - update entry to system version\n";
                std::cout << "s - update entry and discard conflicting elements\n";
                std::cout << "i - import entry\n";
                std::cout << "f - force import overwrites any existing config of equal entry_id\n";
                std::cout << "d - discard entry\n";
                std::cout << "? - print help\n";
                continue;
        }
        break;
    }
    while (1);

    return 0;
}

void
CommandImport::read_archive_entry (struct disir_import *import, int entry,
                                   struct archive_entry_info& entry_info)
{
    enum disir_status status;
    const char *entry_id;
    const char *group_id;
    const char *version;
    const char *info_str;

    status = disir_import_entry_status (import, entry, &entry_id, &group_id, &version, &info_str);

    entry_info.ai_status = status;
    entry_info.ai_group_id = group_id;
    entry_info.ai_version = version;
    entry_info.ai_info = info_str == NULL ? "" : info_str;
    entry_info.ai_entry_id = entry_id;
}

void
CommandImport::print_archive_entries (std::list<struct archive_entry_info>& info)
{
    // find longest entry_id string
    auto longest_entry = std::max_element (info.begin(), info.end(),
                         [](struct archive_entry_info A, struct archive_entry_info B)
                         {
                             return A.ai_entry_id.length() < B.ai_entry_id.length();
                         });
    // find longest group_id string
    auto longest_group = std::max_element (info.begin(), info.end(),
                         [](struct archive_entry_info A, struct archive_entry_info B)
                         {
                             return A.ai_group_id.length() < B.ai_group_id.length();
                         });

    auto entry_id_len = longest_entry->ai_entry_id.length();
    auto group_id_len = longest_group->ai_group_id.length();

    if (entry_id_len < strlen ("entry_id"))
    {
        entry_id_len = strlen ("entry_id");
    }

    if (group_id_len < strlen ("group_id"))
    {
        group_id_len = strlen ("group_id");
    }

    fprintf (stdout,"%-*s  %-*s    %s\n", (int)entry_id_len, "entry_id",
                                          (int)group_id_len, "group_id",
                                          "version");
    for (const auto& entry : info)
    {
        fprintf (stdout, "%-*s  %-*s    %-9s\n", (int)(entry_id_len), entry.ai_entry_id.c_str(),
                                                 (int)(group_id_len), entry.ai_group_id.c_str(),
                                                 entry.ai_version.c_str());
    }
}

int
CommandImport::archive_import_interactive (struct disir_import *import, int entries)
{
    enum disir_status status;
    enum disir_import_option opt;

    for (auto i = 0; i < entries;)
    {
        struct archive_entry_info entry;
        std::string system_entry_version;

        read_archive_entry (import, i, entry);

        fprintf (stdout, "\n%s (%s)\n    group: '%s'\n    status: %s\n",
                         entry.ai_entry_id.c_str(), entry.ai_version.c_str(),
                         entry.ai_group_id.c_str(), disir_status_string (entry.ai_status));

        if (entry.ai_info != "")
        {
             fprintf (stdout, "    import info: %s\n", entry.ai_info.c_str());
        }

        read_config_version (entry.ai_group_id, entry.ai_entry_id, system_entry_version);

        if (system_entry_version.empty() == false &&
            entry.ai_status != DISIR_STATUS_NO_CAN_DO)
        {
            fprintf (stdout, "    system entry version: (%s)\n", system_entry_version.c_str());
        }

        fprintf (stdout, "\n");

        if (read_option (opt))
        {
            // abort
             return(1);
        }

        if (entry.ai_status == DISIR_STATUS_NO_CAN_DO ||
            entry.ai_status == DISIR_STATUS_CONFIG_INVALID)
        {
            if (opt != DISIR_IMPORT_DISCARD)
            {
                std::cout << "\narchive entry cannot be imported, only option is discard"
                          << std::endl;
                continue;
            }
            // as import resolve would disallow even discarding an archive
            // entry with status no can do, we handle it here,
            entry_import_result_format (entry, "", DISIR_IMPORT_DISCARD);
            i++;
            continue;
        }

        status = disir_import_resolve_entry (import, i, opt);
        if (status != DISIR_STATUS_OK)
        {
            if (entry.ai_status == DISIR_STATUS_NO_CAN_DO ||
                entry.ai_status == DISIR_STATUS_CONFIG_INVALID)
            {
                std::cout << "\nerror:  archive entry cannot be imported, only option is discard"
                          << std::endl;
            }

            auto iter = import_option_error.find (std::make_tuple (opt, entry.ai_status, status));
            if (iter != import_option_error.end())
            {
                 std::cout << std::endl << "error:  " <<  iter->second << std::endl;
            }
            continue;
        }

        entry_import_result_format (entry, system_entry_version.c_str(), opt);
        i++;
    }
    return 0;
}

int
CommandImport::read_config_version (std::string& group_id, std::string& entry_id,
                                    std::string& version)
{
    enum disir_status status;
    struct disir_config *config;
    struct disir_version config_version;
    char buf[256];

    status = disir_config_read (m_cli->disir(), group_id.c_str(),
                                entry_id.c_str(), NULL, &config);
    if (status == DISIR_STATUS_OK)
    {
         dc_config_get_version (config, &config_version);

         version = dc_version_string (buf, 256, &config_version);
         disir_config_finished (&config);
         return (0);
    }

    return (1);
}

int
CommandImport::import_all_option (struct disir_import *import, int entries,
                                  enum disir_import_option option)
{
    enum disir_status status;
    enum disir_import_option opt;

    for (auto i = 0; i < entries; i++)
    {
        struct archive_entry_info entry;
        std::string system_entry_version;

        read_archive_entry (import, i, entry);
        m_cli->verbose() << "Resolving entry:\n" << disir_status_string (entry.ai_status) << ":"
                         << "    " << entry.ai_entry_id << std::endl;

        switch (entry.ai_status)
        {
        case DISIR_STATUS_OK:
        {
            opt = DISIR_IMPORT_DO;
            break;
        }
        case DISIR_STATUS_CONFLICTING_SEMVER:
        {
            opt = DISIR_IMPORT_UPDATE;
            if (option == DISIR_IMPORT_DO)
            {
                opt = DISIR_IMPORT_DISCARD;
            }
            if (option == DISIR_IMPORT_FORCE)
            {
                opt = DISIR_IMPORT_FORCE;
            }
            break;
        }
        case DISIR_STATUS_CONFLICT:
        {
            opt = option == DISIR_IMPORT_DO ? DISIR_IMPORT_DISCARD : option;
            break;
        }
        case DISIR_STATUS_NO_CAN_DO:
        case DISIR_STATUS_CONFIG_INVALID:
        {
            opt = DISIR_IMPORT_DISCARD;
            break;
        }
        default:
            opt = DISIR_IMPORT_DISCARD;
            std::cerr << "Got unknown import status: "
                      << disir_status_string (entry.ai_status) << std::endl;
        }

        status = disir_import_resolve_entry (import, i, opt);

        read_config_version (entry.ai_group_id, entry.ai_entry_id, system_entry_version);

        opt = status == DISIR_STATUS_OK ? opt : DISIR_IMPORT_DISCARD;

        entry_import_result_format (entry, system_entry_version.c_str(), opt);
    }

    return 0;
}

void
CommandImport::entry_import_result_format (struct archive_entry_info& entry,
                                           const char *system_entry_version,
                                           enum disir_import_option opt)
{
    enum disir_status status;
    struct disir_mold *mold = NULL;
    struct disir_version version;
    std::string mold_version_string;
    char buf[256];
    std::stringstream ss;

    if (entry.ai_status != DISIR_STATUS_NO_CAN_DO &&
        entry.ai_status != DISIR_STATUS_CONFIG_INVALID)
    {
        status = disir_mold_read (m_cli->disir(), entry.ai_group_id.c_str(),
                                  entry.ai_entry_id.c_str(), &mold);
        if (status != DISIR_STATUS_OK)
        {
            std::cerr << "Failed to read mold (" << entry.ai_entry_id
                      << "): " << disir_status_string (status) << std::endl;
            return;
        }

        status = dc_mold_get_version (mold, &version);
        if (status != DISIR_STATUS_OK)
        {
            std::cerr << "Failed to read mold version: "
                      << disir_status_string (status) << std::endl;
            return;
        }
        mold_version_string = dc_version_string (buf, 256, &version);
    }

    switch (opt)
    {
    case DISIR_IMPORT_UPDATE:
    case DISIR_IMPORT_UPDATE_WITH_DISCARD:
    {
        if (entry.ai_status == DISIR_STATUS_OK ||
            entry.ai_status == DISIR_STATUS_CONFLICTING_SEMVER)
        {
            ss << "archive entry will be updated from " << "(" << entry.ai_version
               << ")" << " to " << "(" << mold_version_string << ")";
        }
        else if (entry.ai_status == DISIR_STATUS_CONFLICT)
        {
            ss << "archive entry " << "(" << entry.ai_version << ") will overwrite system entry "
               << "(" << system_entry_version
               << ") and be updated to " << "(" << mold_version_string << ")";
        }

        m_entries[entry.ai_entry_id] = ss.str();
        break;
    }
    case DISIR_IMPORT_FORCE:
    {
        ss << "archive entry (" << entry.ai_version << ")"
           << " will overwrite system entry (" << system_entry_version << ")";

        m_entries[entry.ai_entry_id] = ss.str();
        break;
    }
    case DISIR_IMPORT_DO:
    {
        ss << "archive entry " << "(" << entry.ai_version << ")"
           << " will be imported without conflicts";

        m_entries[entry.ai_entry_id] = ss.str();
        break;
    }
    case DISIR_IMPORT_DISCARD:
    {
        ss << "archive entry " << "(" << entry.ai_version << ")"
           << " will be discarded";

        m_entries[entry.ai_entry_id] = ss.str();
        break;
    }
    default:
        break;
    }

    if (mold)
        disir_mold_finished (&mold);
}

void
CommandImport::print_import_commit_effects ()
{
    auto longest_entry = std::max_element (m_entries.begin(), m_entries.end(),
                         [](const std::pair<std::string, std::string>& A,
                            const std::pair<std::string, std::string>& B)
                         {
                             return A.first.length() < B.first.length();
                         });

    auto indent = longest_entry->first.length();

    std::cout << "\nthese changes would be made as a result of import:\n" << std::endl;
    for (const auto& entry : m_entries)
    {
        fprintf (stdout, "%-*s  %s\n", (int)indent, entry.first.c_str(), entry.second.c_str());
    }
}

