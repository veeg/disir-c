#include <iostream>
#include <algorithm>
#include <memory>
#include <set>

#include <disir/disir.h>

#include <disir/cli/command_remove.h>
#include <disir/cli/args.hxx>

using namespace disir;

CommandRemove::CommandRemove(void)
    : Command ("remove")
{
}

int
CommandRemove::handle_command (std::vector<std::string> &args)
{
    std::stringstream group_description;
    args::ArgumentParser parser ("Remove a configuration entry");

    setup_parser (parser);
    parser.Prog ("disir remove");

    args::HelpFlag help (parser, "help", "Display the list help menu and exit.",
                         args::Matcher{'h', "help"});

    group_description << "Specify the group to operate on. The loaded default is: "
                      << m_cli->group_id();
    args::ValueFlag<std::string> opt_group_id (parser, "NAME", group_description.str(),
                                               args::Matcher{"group"});
    args::Positional<std::string> opt_entry (parser, "entry",
                                             "Entry to remove");

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
    const auto& entry = args::get (opt_entry);

    status = disir_config_remove (m_cli->disir(), m_cli->group_id().c_str(),
                                  entry.c_str());
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "remove error: " << entry << std::endl;
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
    else
    {
        std::cout << "Removed: " << entry << std::endl;
    }

    return (0);
}
