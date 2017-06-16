#include <iostream>
#include <algorithm>
#include <memory>
#include <limits.h>

#include <disir/disir.h>
#include <disir/fslib/util.h>

#include <disir/cli/command_generate.h>
#include <disir/cli/args.hxx>

using namespace disir;

CommandGenerate::CommandGenerate(void)
    : Command ("generate")
{
}

int
CommandGenerate::handle_command (std::vector<std::string> &args)
{
    std::stringstream group_description;

    args::ArgumentParser parser ("Generate configuration entries to the system.");
    setup_parser (parser);
    parser.Prog ("disir generate");

    args::HelpFlag help (parser, "help", "Display the generate help menu and exit.",
                         args::Matcher{'h', "help"});

    args::ValueFlag<std::string> opt_libdisir (parser, "FILEPATH",
                                               "Generate the libdisir configuration file and exit.",
                                               args::Matcher{"libdisir"});

    group_description << "Specify the group to operate on. The loaded default is: "
                      << m_cli->group_id();
    args::ValueFlag<std::string> opt_group_id (parser, "NAME", group_description.str(),
                                               args::Matcher{"group"});

    args::Flag opt_all (parser, "all",
                         "Generate all available entries. No entry list required.",
                         args::Matcher{"all"});

    args::Flag opt_force (parser, "force",
                         "Force generation of configs. This will overwrite existing configs. Use with care.",
                         args::Matcher{"force"});

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
        std::cerr << "See '" << parser.Prog() << " --help'" << std::endl;
        return (1);
    }
    catch (args::ValidationError e)
    {
        std::cerr << "ValidationError: " << e.what() << std::endl;
        std::cerr << "See '" << parser.Prog() << " --help'" << std::endl;
        return (1);
    }

    if (opt_libdisir)
    {
        auto value = args::get (opt_libdisir);
        return generate_libdisir (value);
    }

    if (opt_entries && opt_all)
    {
        std::cerr << "Supplied both positional list and 'all' flag. Choose one." << std::endl;
        return (-1);
    }

    if (opt_force)
    {
        m_force = true;
    }

    if (opt_group_id && setup_group (args::get(opt_group_id)))
    {
        return (1);
    }

    std::set<std::string> entries_to_generate;
    std::set<std::string> available = available_configs();
    std::set<std::string> namespaces = available_namespaces();
    char namespace_entry[PATH_MAX];
    std::set<std::string> configs;

    // XXX: Ignore if this went shait? Or return in full error?
    list_configs (configs);

    // TODO: Combine force and all to re-generate ALL available molds?

    std::cout << "In group " << m_cli->group_id() << std::endl;
    if (opt_all)
    {
        m_cli->verbose() << "Generating all available entries." << std::endl;
        entries_to_generate = available;
    }
    else if (opt_entries)
    {
        m_cli->verbose() << "Generating entries in user supplied list." << std::endl;
        for (const auto& entry : args::get (opt_entries))
        {
            const char *root = fslib_namespace_entry (entry.c_str(), namespace_entry);

            // Add entry to avalable set if it does not already exist, and
            // it is covered by a namespace mold
            // Either the entry is covered by the top-level namespace (if it exists)
            // or a subnamespace (which namespace_entry does not resolve
            if ((root == NULL && namespaces.count ("/") == 1)
                || (namespaces.count (namespace_entry) != 0 && configs.count (entry) == 0))
            {
                m_cli->verbose() << "Adding entry " << entry << " to available set."
                                 << " (namespace entry)" << std::endl;
                available.insert (entry);
            }

            // Shall we deny attempting to generate this entry?
            if (available.count (entry) == 0 && m_force == false)
            {
                // This key is invalid. Cannot generate
                std::cerr << entry << " is not available for generation." << std::endl
                          << "Use --force to recreate config from mold."
                          << " (This operation will fail if no such mold exist)" << std::endl;
                return (-1);
            }

            // You cannot generate a namespace entry
            if (entry.back() == '/')
            {
                std::cerr << entry << " is not available for generation." << std::endl
                          << "Cannot generate namespace entry." << std::endl;
                return (-1);
            }
            entries_to_generate.insert(entry);
        }
    }

    if (!entries_to_generate.empty())
    {
        return generate_entries (entries_to_generate);
    }

    return list_available_configs (available);
}

int
CommandGenerate::generate_libdisir (std::string &filepath)
{
    enum disir_status status;
    struct disir_mold *mold;
    struct disir_config *config;

    status = disir_libdisir_mold (&mold);
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "Unable to create libdisir mold: "
                  << disir_status_string (status) << std::endl;
        return (-1);
    }
    status = disir_generate_config_from_mold (mold, NULL, &config);
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "Unable to generate libdisir config: "
                  << disir_status_string (status) << std::endl;
        disir_mold_finished (&mold);
        return (-1);
    }

    status = disir_libdisir_config_to_disk (m_cli->disir(), config, filepath.c_str());
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "could not write libdisir config to file: "
                  << disir_status_string (status) << std::endl;
        std::cerr << disir_error (m_cli->disir()) << std::endl;
        disir_mold_finished (&mold);
        disir_config_finished (&config);
        return (-1);
    }

    std::cout << "Generated libdisir config to '" << filepath << "'" << std::endl;
    disir_mold_finished (&mold);
    disir_config_finished (&config);

    return (0);
}

int
CommandGenerate::generate_entries (std::set<std::string>& entries)
{
    enum disir_status status;
    struct disir_mold *mold;
    struct disir_config *config;

    for (const auto entry : entries)
    {
        status = disir_mold_read (m_cli->disir(), m_cli->group_id().c_str(),
                                  entry.c_str(), &mold);
        if (status != DISIR_STATUS_OK)
        {
            // Exit early - we cannot generate
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

        // Generate config - this should never fail, really..
        // TODO: Take version number as cli argument
        status = disir_generate_config_from_mold (mold, NULL, &config);
        if (status != DISIR_STATUS_OK)
        {
            std::cerr << "generation error: " << entry << std::endl;
            if (disir_error (m_cli->disir()) != NULL)
            {
                std::cerr << disir_error (m_cli->disir()) << std::endl;
            }
            else
            {
                std::cerr << "(no error registered)" << std::endl;
            }

            disir_mold_finished (&mold);
            return (-1);
        }

        status = disir_config_write (m_cli->disir(), m_cli->group_id().c_str(),
                                     entry.c_str(), config);
        if (status != DISIR_STATUS_OK)
        {
            std::cerr << "config write error: " << entry << std::endl;
            if (disir_error (m_cli->disir()) != NULL)
            {
                std::cerr << disir_error (m_cli->disir()) << std::endl;
            }
            else
            {
                std::cerr << "(no error registered)" << std::endl;
            }

            disir_config_finished (&config);
            disir_mold_finished (&mold);
            return (-1);
        }

        disir_config_finished (&config);
        disir_mold_finished (&mold);
        std::cout << "  Generated " << entry << std::endl;
    }

    return (0);

}

std::set<std::string>
CommandGenerate::available_configs (void)
{
    enum disir_status status;
    struct disir_entry *mold_entries;
    struct disir_entry *next;
    struct disir_entry *current;
    std::set<std::string> list;

    mold_entries = NULL;

    status = disir_mold_entries (m_cli->disir(), m_cli->group_id().c_str(), &mold_entries);
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "Failed to retrieve available molds: "
                  << disir_error (m_cli->disir()) << std::endl;
        return list;
    }

    current = mold_entries;
    while (current != NULL)
    {
        next = current->next;

        // Ignore namespace entries
        if (current->flag.DE_NAMESPACE_ENTRY)
        {
            current = next;
            continue;
        }

        status = disir_config_query (m_cli->disir(), m_cli->group_id().c_str(),
                                     current->de_entry_name, NULL);
        if (status == DISIR_STATUS_NOT_EXIST)
        {
            list.insert (std::string(current->de_entry_name));
        }

        disir_entry_finished (&current);
        current = next;
    }

    return list;
}

std::set<std::string>
CommandGenerate::available_namespaces (void)
{
    enum disir_status status;
    struct disir_entry *mold_entries;
    struct disir_entry *next;
    struct disir_entry *current;
    std::set<std::string> list;

    mold_entries = NULL;

    status = disir_mold_entries (m_cli->disir(), m_cli->group_id().c_str(), &mold_entries);
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "Failed to retrieve available molds: "
                  << disir_error (m_cli->disir()) << std::endl;
        return list;
    }

    current = mold_entries;
    while (current != NULL)
    {
        next = current->next;

        if (current->flag.DE_NAMESPACE_ENTRY)
        {
            list.insert (std::string(current->de_entry_name));
        }

        disir_entry_finished (&current);
        current = next;
    }

    return list;
}

int
CommandGenerate::list_available_configs (std::set<std::string>& entries)
{
    std::set<std::string> namespaces = available_namespaces();

    if (!namespaces.empty())
    {
        std::cout << "  There are " << namespaces.size() << " namespaces available." << std::endl;
        std::cout << std::endl;

        for (const auto& ns : namespaces)
        {
            std::cout << "    " << ns << std::endl;
        }
        std::cout << std::endl;
    }

    if (entries.empty())
    {
        std::cout << "  There are no available entries to generate." << std::endl;
        return (0);
    }

    std::cout << "  There are " << entries.size() << " available entries to generate." << std::endl;
    std::cout << std::endl;
    for (const auto& entry : entries)
    {
        std::cout << "    " << entry << std::endl;
    }
    std::cout << std::endl;

    return (0);
}

