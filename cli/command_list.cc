#include <iostream>
#include <algorithm>
#include <memory>
#include <set>

#include <disir/disir.h>

#include <disir/cli/command_list.h>
#include <disir/cli/args.hxx>

using namespace disir;

CommandList::CommandList(void)
    : Command ("list")
{
}

int
CommandList::handle_command (std::vector<std::string> &args)
{
    std::stringstream group_description;
    args::ArgumentParser parser ("List available configuration entries.");

    setup_parser (parser);
    parser.Prog ("disir list");

    args::HelpFlag help (parser, "help", "Display the list help menu and exit.",
                         args::Matcher{'h', "help"});
    group_description << "Specify the group to operate on. The loaded default is: "
                      << m_cli->group_id();
    args::ValueFlag<std::string> opt_group_id (parser, "NAME", group_description.str(),
                                               args::Matcher{"group"});

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

    if (opt_group_id && setup_group (args::get(opt_group_id)))
    {
        return (1);
    }

    std::set<std::string> list;
    if (list_configs(list))
    {
        return (-1);
    }

    std::cout << "In group " << m_cli->group_id() << std::endl;
    if (list.empty())
    {
        std::cout << "  There are no available configs." << std::endl;
        return (0);
    }

    std::cout << "  There are " << list.size() << " available config(s)." << std::endl;
    std::cout << std::endl;
    for (const auto& entry : list)
    {
        std::cout << "    " << entry << std::endl;
    }
    std::cout << std::endl;

    return (0);
}

