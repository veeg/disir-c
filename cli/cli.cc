#include <sstream>
#include <iostream>
#include <list>
#include <algorithm>
#include <numeric>
#include <unistd.h>

#include <disir/disir.h>
#include <disir/version.h>
#include <disir/plugin.h>

#include <disir/cli/cli.h>
#include <disir/cli/args.hxx>

// Include the commands used as default
#include <disir/cli/command_list.h>
#include <disir/cli/command_generate.h>
#include <disir/cli/command_verify.h>
#include <disir/cli/command_dump.h>

using namespace disir;

//
// source:
//  https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance
//
static int
levenshtein_distance(const std::string &s1, const std::string &s2)
{
    int s1len = s1.size();
    int s2len = s2.size();

    auto column_start = (decltype(s1len))1;

    auto column = new decltype(s1len)[s1len + 1];
    std::iota(column + column_start, column + s1len + 1, column_start);

    for (auto x = column_start; x <= s2len; x++) {
        column[0] = x;
        auto last_diagonal = x - column_start;
        for (auto y = column_start; y <= s1len; y++) {
            auto old_diagonal = column[y];
            auto possibilities = {
                column[y] + 1,
                column[y - 1] + 1,
                last_diagonal + (s1[y - 1] == s2[x - 1]? 0 : 1)
            };
            column[y] = std::min(possibilities);
            last_diagonal = old_diagonal;
        }
    }
    auto result = column[s1len];
    delete[] column;
    return result;
}

Cli::Cli (const std::string &program_name)
    : m_program_name (program_name),
      m_config_filepath ("/etc/disir/disir.conf"),
      m_ostream_sink(0)
{
    if (access (m_config_filepath.c_str(), F_OK | R_OK) == -1)
    {
        m_config_filepath = "";
    }
}

std::ostream& Cli::verbose (void)
{
    if (m_verbose)
        return std::cout;
    else
        return m_ostream_sink;
}

int
Cli::parse_command_line_args (std::vector<std::string> &args)
{
    std::ostringstream os;

    available_commands (os);
    args::ArgumentParser parser ("Disir configuration entry management utility",
                                 os.str ());
    parser.Prog (m_program_name);
    parser.helpParams.progindent = 0;
    parser.helpParams.progtailindent = 2;
    parser.helpParams.descriptionindent = 2;
    parser.helpParams.flagindent = 2;
    parser.helpParams.eachgroupindent = 0;
    parser.helpParams.helpindent = 28;
    parser.helpParams.showTerminator = false;
    parser.ProglinePostfix ("[<args>]");

    // Add flags/arguments
    args::HelpFlag opt_help (parser, "help", "Display this help menu and exit.",
                        args::Matcher{'h', "help"});
    args::Flag opt_version (parser, "version", "Show the disir version and exit.",
                           args::Matcher{'v', "version"});
    args::Flag opt_verbose (parser, "verbose", "Enable verbose cli output.",
                        args::Matcher{'V', "verbose"});
    args::ValueFlag<std::string> opt_config (parser, "PATH", "libdisir config filepath",
                                         args::Matcher{'c', "config"});
    args::Positional<std::string> command (parser, "COMMAND", "The disir command to execute.");

    m_help_text = parser.Help();

    try
    {
        parser.ParseArgs (args);
    }
    catch (args::Help)
    {
        if (!command)
        {
            m_handle_help = true;
            return (0);
        }
    }
    catch (args::ParseError e)
    {
        if (!command)
        {
            std::cerr << "ParserError: " << e.what () << std::endl;
            std::cerr << "See '" << m_program_name << " --help'" << std::endl;
            return (1);
        }
    }
    catch (args::ValidationError e)
    {
        if (!command)
        {
            std::cerr << "ValidationError: " << e.what () << std::endl;
            std::cerr << "See '" << m_program_name << " --help'" << std::endl;
            return (1);
        }
    }
    if (opt_verbose)
    {
        m_verbose = true;
        verbose() << "Enabling verbose output" << std::endl;
    }

    if (opt_version)
        m_handle_version = true;

    if (command)
        m_active_command = args::get (command);

    if (opt_config)
    {
        m_config_filepath = args::get (opt_config);
    }

    return 0;
}

int
Cli::initialize_disir ()
{
    enum disir_status status;
    const char *filepath = NULL;
    const char *default_filepath = "/etc/disir/libdisir.toml";

    // 1. User provided filepath exists:
    // 2. User provider filepath does not exist, default exists
    // 3. user provided filepath does not exist, default does not exist
    // 4. User did not proivde filepath, default exists
    // 5. User did not provide fileoath, default does not exist

    if (m_config_filepath.empty())
    {
        // TODO: Output that we are using internally generated config if debug
        verbose() << "No user-provided libdisir config provided. Using default: "
                  << default_filepath << std::endl;
        filepath = default_filepath;
    }
    else if (access(m_config_filepath.c_str(), F_OK) != -1)
    {
        // TODO: Output that we are using user-provide config, since it exists
        verbose() << "Using user-provided libdisir config: " << m_config_filepath << std::endl;
        filepath = m_config_filepath.c_str();
    }
    else
    {
        // TODO: Output that user provided entry does not exist. Using default
        verbose() << "User-provided libdisir config filepath is not accessible. Using default: "
                  << default_filepath << std::endl;
        filepath = default_filepath;
    }

    // Pointer comparison - checking if default filepath exists
    if (filepath == default_filepath)
    {
        if (access (default_filepath, F_OK) == -1)
        {
            // TODO: Output that default filepath does not exist. Using internallt generated
            verbose() << "Default libdisir config file is not accessible. Using internally generated."
                      << std::endl;
            filepath = NULL;
        }
    }

    status = disir_instance_create (filepath, NULL, &m_disir);
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "Failed to initialize libdisir: "
                  << disir_status_string (status) << std::endl;
        if (status == DISIR_STATUS_LOAD_ERROR)
        {
            std::cerr << "A plugin failed to dynamically load." << std::endl;
        }
        if (status == DISIR_STATUS_PLUGIN_ERROR)
        {
            std::cerr << "A plugin failed to initialize." << std::endl;
        }
        if (status == DISIR_STATUS_CONFIG_INVALID)
        {
            std::cerr << "Supplied configuration is incomplete." << std::endl;
        }

        std::cerr << "See logfile for more information." << std::endl;
        return (-1);
    }

    list_groups (m_groups);
    if (m_groups.size() > 0)
    {
        m_group_id = std::string (m_groups.front());
        verbose() << "Setting default group id to: " << m_group_id << std::endl;
    }

    return (0);
}

int
Cli::list_groups (std::list<std::string>& list)
{
    enum disir_status status;
    struct disir_plugin *plugins;
    struct disir_plugin *next;
    struct disir_plugin *current;

    status = disir_plugin_registered (disir(), &plugins);
    if (status != DISIR_STATUS_OK)
        return (-1);

    current = plugins;
    while (current != NULL)
    {
        next = current->next;

        if (std::find(list.begin(), list.end(), std::string(current->pl_group_id)) == list.end())
        {
            list.push_back (std::string(current->pl_group_id));
        }

        disir_plugin_finished (&current);
        current = next;
    }

    return (0);
}

void
Cli::add_default_commands (void)
{
    std::shared_ptr<Command> command_ptr;

    command_ptr = std::make_shared<CommandList> ();
    add_command (command_ptr);

    command_ptr = std::make_shared<CommandGenerate> ();
    add_command (command_ptr);

    command_ptr = std::make_shared<CommandVerify> ();
    add_command (command_ptr);

    command_ptr = std::make_shared<CommandDump> ();
    add_command (command_ptr);
}

void
Cli::add_command (std::shared_ptr<Command> command)
{
    command->m_cli = shared_from_this ();
    m_commands.emplace (command->m_name, command);
}

struct disir_instance *
Cli::disir (void)
{
    return m_disir;
}

std::string&
Cli::group_id (void)
{
    return m_group_id;
}

std::string&
Cli::group_id (std::string& id)
{
    m_group_id = id;
    return m_group_id;
}

void
Cli::available_commands (std::ostringstream &os)
{
    os << "Available commands are:" << std::endl;
    for (auto it = m_commands.begin (); it != m_commands.end (); ++it)
    {
        os << "- " << it->first << std::endl;
    }
}

void
Cli::version (std::ostringstream &os)
{
    os << "disir version: " << libdisir_version_string
       << " (build " << libdisir_build_string << ")" << std::endl;
}

void
Cli::no_matching_command (void)
{
    std::ostringstream os;

    // No matching command register - do levenshtein fuzzy matching
    std::cerr << m_program_name << ": '" << m_active_command << "' is not a "
              << m_program_name << " command."
              << " See '" << m_program_name << " --help'." << std::endl;

    os << std::endl;
    if (report_related (m_active_command, os))
    {
        std::cerr << os.str ();
    }
}

bool
Cli::report_related (const std::string& command_name, std::ostringstream& os)
{
    int res;
    int matching_boundrary = 3;
    std::list<std::string> matching;
    std::list<std::string>::iterator li;
    std::map<std::string, std::shared_ptr<Command>>::iterator it;

    for (it = m_commands.begin (); it != m_commands.end (); ++it)
    {
        res = levenshtein_distance (command_name, it->first);
        if (res <= matching_boundrary)
        {
            matching.push_back (it->first);
        }
    }

    if (matching.empty ())
    {
        return (false);
    }

    os << "Did you mean this?" << std::endl;
    for (li = matching.begin (); li != matching.end (); ++li)
    {
        os << "        " << *li << std::endl;
    }

    return (true);
}

int
Cli::run (std::vector<std::string> &args)
{
    int res;
    std::ostringstream os;
    std::map<std::string, std::shared_ptr<Command>>::iterator it;

    // non-zero result implies parse/usage error
    res = parse_command_line_args (args);
    if (res)
    {
        return (res);
    }

    // If the internal flag to output version number is set, we output that and exit
    if (m_handle_version)
    {
        os.str ("");
        os.clear ();
        version (os);
        std::cout << os.str ();
        return (0);
    }

    // No command specified (or help requred). Exit with help.
    if (m_active_command.empty() || m_handle_help)
    {
        // TODO: Use ostream
        std::cout << m_help_text;
        return (0);
    }

    if (initialize_disir ())
    {
        return (-1);
    }

    // Find the requested command
    it = m_commands.find (m_active_command);
    if (it != m_commands.end ())
    {
        // pop arguments until argument matches command
        std::string entry;
        do
        {
            entry = args[0];
            args.erase(args.begin());
        } while (entry.compare (m_active_command) != 0);

        return it->second->handle_command (args);
    }
    else
    {
        // TODO: ostream
        no_matching_command ();
        return (1);
    }
}

