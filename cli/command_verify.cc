#include <iostream>
#include <algorithm>
#include <memory>
#include <set>

#include <disir/disir.h>
#include <disir/fslib/util.h>
#include <disir/fslib/json.h>

#include <disir/cli/command_verify.h>
#include <disir/cli/args.hxx>

using namespace disir;

CommandVerify::CommandVerify(void)
    : Command ("verify")
{
}

int
CommandVerify::handle_command (std::vector<std::string> &args)
{
    std::stringstream group_description;
    args::ArgumentParser parser ("Verify configuration entries and their associated molds.");

    setup_parser (parser);
    parser.Prog ("disir verify");

    args::HelpFlag help (parser, "help", "Display the list help menu and exit.",
                         args::Matcher{'h', "help"});

    group_description << "Specify the group to operate on. The loaded default is: "
                      << m_cli->group_id();
    args::ValueFlag<std::string> opt_group_id (parser, "NAME", group_description.str(),
                                               args::Matcher{"group"});

    args::ValueFlag<std::string> opt_text_mold (parser, "TEXT MOLD",
                                                "Verify mold from disk.",
                                                args::Matcher{"text-mold"});
    args::PositionalList<std::string> opt_entries (parser, "entry",
                                                   "A list of entries to verify.");

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
        std::cerr << "See '" << m_cli->m_program_name << " --help'" << std::endl;
        return (1);
    }
    catch (args::ValidationError e)
    {
        std::cerr << "ValidationError: " << e.what() << std::endl;
        std::cerr << "See '" << m_cli->m_program_name << " --help'" << std::endl;
        return (1);
    }


    if (opt_text_mold)
    {
        enum disir_status status;
        struct disir_mold *mold = NULL;
        struct stat statbuf;
        std::string filepath_mold;

        filepath_mold = args::get (opt_text_mold);

        std::cout << "mold text: " << filepath_mold << std::endl;
        status = fslib_stat_filepath (m_cli->disir(), filepath_mold.c_str(), &statbuf);
        if (status != DISIR_STATUS_OK)
        {
            std::cout << "  " << disir_error (m_cli->disir()) << std::endl;
            return (1);
        }

        // TODO: Check file extension - switch on available molds
        // XXX Hardcode to json unserialize for now
        status = dio_json_unserialize_mold_filepath (m_cli->disir(), filepath_mold.c_str(), &mold);
        print_verify (status, filepath_mold.c_str(), NULL, mold);

        // TODO: Take entries arguments and verify them as config with this mold ( if the mold is OK)

        // Cleanup
        if (mold)
        {
            disir_mold_finished (&mold);
        }

        return (0);
    }

    if (opt_group_id && setup_group (args::get(opt_group_id)))
    {
        return (1);
    }

    // Get the set of entries to verify
    std::set<std::string> entries_to_verify;
    if (opt_entries)
    {
        m_cli->verbose() << "Verifying entries in user supplied list." << std::endl;
        for (const auto& entry : args::get (opt_entries))
        {
            entries_to_verify.insert (entry);
        }
    }
    else
    {
        m_cli->verbose() << "Verifying all available entries." << std::endl;

        enum disir_status status;
        struct disir_entry *config_entries;
        struct disir_entry *next;
        struct disir_entry *current;

        status = disir_config_entries (m_cli->disir(), m_cli->group_id().c_str(), &config_entries);
        if (status != DISIR_STATUS_OK)
        {
            std::cerr << "Failed to retrieve available configs: "
                      << disir_error (m_cli->disir()) << std::endl;
            return (-1);
        }

        current = config_entries;
        while (current != NULL)
        {
            next = current->next;

            entries_to_verify.insert (std::string(current->de_entry_name));

            disir_entry_finished (&current);
            current = next;
        }
    }

    // Iterate each entry, attempt to verify and print result
    std::cout << "In group " << m_cli->group_id() << std::endl;
    if (entries_to_verify.empty())
    {
        std::cout << "  There are no available configs." << std::endl;
        return (0);
    }

    m_cli->verbose() << "There are " << entries_to_verify.size()
                     << " entries to verify." << std::endl;
    std::cout << std::endl;
    for (const auto& entry : entries_to_verify)
    {
        // We simply read the config entry - the return status shall indicate whether it is invalid or not
        enum disir_status status;
        struct disir_config *config = NULL;

        status = disir_config_read (m_cli->disir(), m_cli->group_id().c_str(),
                                    entry.c_str(), NULL, &config);

        print_verify (status, entry.c_str(), config, NULL);
        if (config)
            disir_config_finished (&config);
    }
    std::cout << std::endl;

    return (0);
}

void
CommandVerify::print_verify (enum disir_status status, const char *entry,
                             struct disir_config *config,
                             struct disir_mold *mold)
{
    struct disir_collection *collection = NULL;
    struct disir_context *context = NULL;
    char *resolved_name = NULL;

    if (status == DISIR_STATUS_OK)
    {
        std::cout << "    OK:      " << entry << std::endl;
    }
    else if (status == DISIR_STATUS_INVALID_CONTEXT)
    {
        std::cout << "    INVALID: " << entry << std::endl;

        if (config)
            status = disir_config_valid (config, &collection);
        else if (mold)
            status = disir_mold_valid (mold, &collection);
        if (collection)
        {
            do
            {
                status = dc_collection_next (collection, &context);
                if (status == DISIR_STATUS_EXHAUSTED)
                    break;
                if (status != DISIR_STATUS_OK)
                {
                    std::cerr << "failed to retrieve collection item: "
                              << disir_status_string (status) << std::endl;
                    break;
                }

                status = dc_resolve_root_name (context, &resolved_name);
                const char *name;
                if (status != DISIR_STATUS_OK)
                {
                    name = "UNRESOLVED";
                }
                else
                {
                    name = resolved_name;
                }

                if (dc_context_error (context))
                {
                    std::cout << "               " << name << ": "
                              << dc_context_error (context) << std::endl;
                }
                else if (dc_context_type (context) == DISIR_CONTEXT_KEYVAL)
                {
                    std::cout << "               " << name << ": "
                              << "(entry missing error)" << std::endl;
                }
                if (resolved_name != NULL)
                {
                    free (resolved_name);
                }
                dc_putcontext (&context);
            } while (1);

            if (context)
            {
                dc_putcontext (&context);
            }
            if (collection)
            {
                dc_collection_finished (&collection);
            }
        }
    }
    else
    {
        std::cout << "    ERROR:   " << entry << std::endl;
        std::cout << "               " << disir_status_string (status) << std::endl;
        if (disir_error (m_cli->disir()) != NULL)
        {
            std::cout << "             " << disir_error (m_cli->disir()) << std::endl;
        }
        else
        {
            std::cout << "             (no error registered)" << std::endl;
        }
    }
}
