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
    args::ArgumentParser parser ("Generate configuration entries to the system.");
    setup_parser (parser);
    parser.Prog ("disir import");

    args::HelpFlag help (parser, "help", "Display the generate help menu and exit.",
                         args::Matcher{'h', "help"});


    args::ValueFlag<std::string> opt_archive_path (parser, "ARCHIVE",
                                                   "Filepath to disir archive. (REQUIRED)",
                                                   args::Matcher{"archive"});

    args::Group group(parser, "These flags are all exclusive:", args::Group::Validators::Xor);

    args::Flag opt_interactive (group, "interactive",
                                "Import and resolve archive entries one by one.",
                                args::Matcher{"interactive", 'i'});

    args::Flag opt_force (group, "force",
                          "Force import (importable) entries in archive - archive wins.",
                          args::Matcher{"force"});

    args::Flag opt_soft (group, "soft",
                         "System + update entries that are not up-to-date.",
                         args::Matcher{"soft"});

    args::Flag opt_system (group, "system",
                           "Import all entries which do not conflict with system - system wins.",
                           args::Matcher{"system"});

    args::Flag opt_list (group, "list", "Print configuration entries in archive.",
                         args::Matcher{"list"});

    args::Flag opt_inspect (group, "inspect", "Check all entries for import.",
                            args::Matcher{"inspect"});

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
        std::cout << "ParseError: " << e.what() << std::endl;
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
        int ret = -1;
        int entries;

        status = disir_archive_import (m_cli->disir(), archive_path.c_str(), &import, &entries);
        if (status != DISIR_STATUS_OK)
        {
            std::cerr << "Could not import archive (" << archive_path << "): "
                      << disir_error (m_cli->disir()) << std::endl;
            return (-1);
        }

        if (opt_list || opt_inspect)
        {
            auto inspect = !!opt_inspect;
            std::cout << "There are " << entries << " entries in the archive:\n" << std::endl;
            for (auto i = 0; i < entries; i++)
            {
                read_archive_entry (import, i, info);
            }
            print_archive_entries (info, inspect);
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

        option = ret == 0 ? DISIR_IMPORT_DO : DISIR_IMPORT_DISCARD;

        status = disir_import_finalize (m_cli->disir(), option, &import, &report);
        if (status != DISIR_STATUS_OK)
        {
            if (disir_error (m_cli->disir()))
            {
                std::cout << disir_error (m_cli->disir()) << std::endl;
            }
            return (1);
        }
        if (report)
        {
            std::cout << "Import summary: " << std::endl;
            for (auto i = 0; i < report->ir_entries; i++)
            {
                std::cout << "      " << report->ir_entry[i] << std::endl;
            }
            disir_import_report_destroy (&report);
        }
    }
    else
    {
        std::cerr << "path to disir archive must be set '--archive'" << std::endl;
    }

    return 0;
}

int
CommandImport::read_option (enum disir_import_option& option)
{
    char opt;

    do
    {
        std::cout << "import entry [a,u,i,f,d,s,h]? ";
        std::string input;
        std::getline (std::cin, input);

        if (input.length() > 1)
        {
            std::cout << "Invalid option" << std::endl;
            continue;
        }

        opt = input[0];
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
                std::cout << "Abort (a)\n";
                std::cout << "Import update  (u)\n";
                std::cout << "  Bring config up-to-date while keeping keyvals that are nondefault\n";
                std::cout << "Import with discard (soft) (s)\n";
                std::cout << "  Bring config up-to-date but discard keyvals that casuse conflicts"
                          << " on higher version of a mold\n";
                std::cout << "Import do      (i)\n";
                std::cout << "  Import entry that has import status OK or"
                          << " config that only conflicts with a higher mold version\n";
                std::cout << "Import force   (f)\n";
                std::cout << "  Force import - overwrites any existing config of equal entry_id\n";
                std::cout << "Import discard (d)\n";
                std::cout << "  Deselect entry for import\n";
                continue;
            default:
                std::cout << "Invalid option" << std::endl;
                continue;
        }
        break;
    }
    while (1);

    return 0;
}

void
CommandImport::read_archive_entry (struct disir_import *import, int entry,
                                   std::list<struct archive_entry_info>& info)
{
    enum disir_status status;
    struct archive_entry_info entry_info;
    const char *entry_id;
    const char *group_id;
    const char *version;
    const char *info_str;

    status = disir_import_entry_status (import, entry, &entry_id, &group_id, &version, &info_str);

    entry_info.ai_status = disir_status_string (status);
    entry_info.ai_group_id = group_id;
    entry_info.ai_version = version;
    entry_info.ai_info = info_str == NULL ? "" : info_str;
    entry_info.ai_entry_id = entry_id;

    info.push_front (entry_info);
}

void
CommandImport::print_archive_entries (std::list<struct archive_entry_info>& info, bool inspect)
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
    // fint longest status string
    auto longest_status = std::max_element (info.begin(), info.end(),
                          [](struct archive_entry_info A, struct archive_entry_info B)
                          {
                              return A.ai_status.length() < B.ai_status.length();
                          });

    auto entry_id_len = longest_entry->ai_entry_id.length();
    auto group_id_len = longest_group->ai_group_id.length();
    auto status_len = longest_status->ai_status.length();

    if (entry_id_len < strlen ("entry_id"))
    {
        entry_id_len = strlen ("entry_id");
    }

    if (group_id_len < strlen ("group_id"))
    {
        group_id_len = strlen ("group_id");
    }

    if (status_len < strlen ("status"))
    {
        status_len = strlen ("status");
    }

    fprintf (stdout,"%-*s  %-*s    %s", (int)entry_id_len, "entry_id",
                                        (int)group_id_len, "group_id",
                                        "version");
    if (inspect)
    {
        fprintf (stdout, "  %-*s    %s", (int)status_len, "status", "info");
    }

    fprintf (stdout, "\n");

    for (const auto& entry : info)
    {
        fprintf (stdout, "%-*s  %-*s    %-9s", (int)(entry_id_len), entry.ai_entry_id.c_str(),
                                               (int)(group_id_len), entry.ai_group_id.c_str(),
                                               entry.ai_version.c_str());

        if (inspect)
        {
            fprintf (stdout, "%-*s    %s", (int)(status_len), entry.ai_status.c_str(),
                                           entry.ai_info.c_str());
        }
        fprintf (stdout, "\n");
    }
}

int
CommandImport::archive_import_interactive (struct disir_import *import, int entries)
{
    enum disir_status status;
    enum disir_status valid;
    enum disir_import_option opt;
    struct disir_version version;
    struct disir_config *config;
    const char *entry_id;
    const char *group_id;
    const char *entry_version;
    const char *info;
    char buf[100];

    for (auto i = 0; i < entries;)
    {
       valid = disir_import_entry_status (import, i, &entry_id, &group_id, &entry_version, &info);

       fprintf (stdout, "\n%s (%s)\n    group: '%s'\n    status: %s\n",
                        entry_id, entry_version, group_id, disir_status_string (valid));

       if (info)
       {
            fprintf (stdout, "    import info: %s\n", info);
       }

       status = disir_config_read (m_cli->disir(), group_id, entry_id, NULL, &config);
       if (status == DISIR_STATUS_OK)
       {
            dc_config_get_version (config, &version);
            fprintf (stdout, "    system entry version: (%s)\n",
                                  dc_version_string (buf, 100, &version));
            disir_config_finished (&config);
       }

       fprintf (stdout, "\n");

       if (read_option (opt))
       {
           // abort
            return(1);
       }

       status = disir_import_resolve_entry (import, i, opt);
       if (status != DISIR_STATUS_OK)
       {
           if (valid != DISIR_STATUS_NO_CAN_DO)
           {
                if (status == DISIR_STATUS_NO_CAN_DO)
                {
                    std::cerr << "\nCannot resolve entry with provided option" << std::endl;
                }
                else if (status == DISIR_STATUS_RESTRICTION_VIOLATED)
                {
                    std::cerr << "An update resulted in volation of restrictions introduced"
                              << " at a higher version. use UPDATE_WITH_DISCARD (s) to resolve\n";
                }
           }
           else
           {
                std::cerr << entry_id << " cannot be imported" << std::endl;
                std::cerr << "UNRESOLVABLE:     " << info << std::endl;
           }
           continue;
       }

       i++;
    }
    return 0;
}

int
CommandImport::import_all_option (struct disir_import *import, int entries,
                                  enum disir_import_option option)
{
    enum disir_status status;
    enum disir_status valid;
    enum disir_import_option opt;
    const char *entry_id;
    const char *group_id;
    const char *version;
    const char *error;

    for (auto i = 0; i < entries; i++)
    {
        valid = disir_import_entry_status (import, i, &entry_id, &group_id, &version, &error);
        m_cli->verbose() << "Resolving entry:\n" << disir_status_string (valid) << ":"
                         << "    " << entry_id << std::endl;

        switch (valid)
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
            std::cerr << "Got unknown import status: " << disir_status_string (status) << std::endl;
        }

        status = disir_import_resolve_entry (import, i, opt);
        if (status != DISIR_STATUS_OK)
        {
            if (valid != DISIR_STATUS_NO_CAN_DO)
            {
                 if (status == DISIR_STATUS_NO_CAN_DO)
                 {
                     m_cli->verbose() << "cannot resolve entry with provided option" << std::endl;
                 }
                 else if (status == DISIR_STATUS_RESTRICTION_VIOLATED)
                 {
                     m_cli->verbose() << "An update resulted in volation of restrictions introduced"
                                      << " at a higher version." << std::endl;
                 }
            }
        }
    }

    return 0;
}

