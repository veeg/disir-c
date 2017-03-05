#include <iostream>
#include <algorithm>
#include <memory>
#include <set>

#include <disir/disir.h>

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
    args::ArgumentParser parser ("verify configs and molds");
    parser.helpParams.showTerminator = false;
    parser.Prog ("disir verify");

    args::HelpFlag help (parser, "help", "Display the list help menu and exit.",
                         args::Matcher{'h', "help"});

    args::PositionalList<std::string> opt_entries (parser, "entry",
                                                   "A list of entries to generate configs for.");

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

    // TODO: Expose group selection.

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
        if (status == DISIR_STATUS_OK)
        {
            std::cout << "    OK:      " << entry << std::endl;
        }
        else if (status == DISIR_STATUS_INVALID_CONTEXT)
        {
            struct disir_collection *collection = NULL;
            struct disir_context *context = NULL;
            std::cout << "    INVALID: " << entry << std::endl;

            // We need to iterate all child contexts of config, to find where the dirty ones are
            // Currently, disir_config_valid does this for us.
            status = disir_config_valid (config, &collection);
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

                    if (dc_context_error (context))
                    {
                        std::cout << "               " << dc_context_error (context) << std::endl;
                    }
                    else if (dc_context_type (context) == DISIR_CONTEXT_KEYVAL)
                    {
                        std::cout << "               (keyval entry missing error)" << std::endl;
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

        if (config)
            disir_config_finished (&config);
    }
    std::cout << std::endl;

    return (0);
}

