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
    args::ArgumentParser parser ("list configuration files, molds or I/O plugins");
    parser.helpParams.showTerminator = false;
    parser.Prog ("disir list");

    args::HelpFlag help (parser, "help", "Display the list help menu and exit.",
                         args::Matcher{'h', "help"});

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
    std::set<std::string> list;

    list = list_configs();

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

