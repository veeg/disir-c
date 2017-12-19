#include <iostream>
#include <algorithm>
#include <memory>
#include <set>

#include <disir/disir.h>

#include <disir/cli/command_dump.h>
#include <disir/cli/args.hxx>

using namespace disir;

CommandDump::CommandDump(void)
    : Command ("dump")
{
}

int
CommandDump::handle_command (std::vector<std::string> &args)
{
    std::stringstream group_description;
    args::ArgumentParser parser ("Dump a configuration entry to stdout");

    setup_parser (parser);
    parser.Prog ("disir dump");

    args::HelpFlag help (parser, "help", "Display the list help menu and exit.",
                         args::Matcher{'h', "help"});

    group_description << "Specify the group to operate on. The loaded default is: "
                      << m_cli->group_id();
    args::ValueFlag<std::string> opt_group_id (parser, "NAME", group_description.str(),
                                               args::Matcher{"group"});
    args::Positional<std::string> opt_entry (parser, "entry",
                                             "Entry to dump values of");

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
        std::cerr << "See '" << m_cli->m_program_name << " --help'" << std::endl;
        return (1);
    }
    catch (args::ValidationError e)
    {
        std::cerr << "ValidationError: " << e.what() << std::endl;
        std::cerr << "See '" << m_cli->m_program_name << " --help'" << std::endl;
        return (1);
    }

    if (!opt_entry)
    {
        std::cerr << "missing required argument [entry]" << std::endl;
        std::cout << parser;
        return (1);
    }

    if (opt_group_id && setup_group (args::get(opt_group_id)))
    {
        return (1);
    }
    std::cout << "In group " << m_cli->group_id() << std::endl;
    std::cout << std::endl;

    enum disir_status status;
    struct disir_config *config;
    const auto& entry = args::get (opt_entry);

    status = disir_config_read (m_cli->disir(), m_cli->group_id().c_str(),
                                entry.c_str(), NULL, &config);
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "mold read error: " << entry << std::endl;
        if (disir_error (m_cli->disir()) != NULL)
        {
            std::cerr << disir_error (m_cli->disir()) << std::endl;
        }
        else
        {
            std::cerr << "(no error registered)" << std::endl;
        }
        return (-1);
    }

    output_config_context (dc_config_getcontext (config), 2);

    return (0);
}

int
CommandDump::output_config_context (struct disir_context *context, int indent)
{
    enum disir_status status;
    struct disir_collection *collection;
    struct disir_context *element;
    const char *element_name;

    status = dc_get_elements (context, &collection);
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "error: unable to get elements..." << std::endl;
        return (-1);
    }

    do
    {
        status = dc_collection_next (collection, &element);
        if (status == DISIR_STATUS_EXHAUSTED)
        {
            status = DISIR_STATUS_OK;
            break;
        }

        if (status != DISIR_STATUS_OK)
        {
            break;
        }

        status = dc_get_name (element, &element_name, NULL);
        if (status != DISIR_STATUS_OK)
        {
            element_name = "[UNRESOLVED]";
        }

        std::cout << std::string (indent, ' ');
        std::cout << element_name << ": ";

        if (dc_context_type (element) == DISIR_CONTEXT_KEYVAL)
        {
            char buffer[1024];
            int32_t output_size;
            status = dc_get_value (element, 1024, buffer, &output_size);
            if (status != DISIR_STATUS_OK)
            {
                std::cerr << "error getting value...";
            }
            else
            {
                std::cout << buffer;
            }

            std::cout << std::endl;
        }
        else if (dc_context_type (element) == DISIR_CONTEXT_SECTION)
        {
            std::cout << std::endl;

            // TODO: Handle return code
            output_config_context (element, indent + 2);
        }

        dc_putcontext (&element);
    } while (1);

    dc_collection_finished (&collection);

    return (0);
}

