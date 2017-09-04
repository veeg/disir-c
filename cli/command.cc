#include <disir/cli/command.h>
#include <disir/cli/cli.h>

#include <iostream>

using namespace disir;

Command::Command (std::string name)
    : m_name (name)
{
}

void
Command::setup_parser (args::ArgumentParser& parser)
{
    parser.helpParams.progindent = 0;
    parser.helpParams.progtailindent = 2;
    parser.helpParams.descriptionindent = 2;
    parser.helpParams.flagindent = 2;
    parser.helpParams.eachgroupindent = 0;
    parser.helpParams.helpindent = 28;
    parser.helpParams.showTerminator = false;
}

int
Command::list_configs (std::set<std::string>& list)
{
    enum disir_status status;
    struct disir_entry *config_entries;
    struct disir_entry *next;
    struct disir_entry *current;

    status = disir_config_entries (m_cli->disir(), m_cli->group_id().c_str(), &config_entries);
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "error querying available configs: "
                  << disir_error (m_cli->disir()) << std::endl;
        return (-1);
    }

    current = config_entries;
    while (current != NULL)
    {
        next = current->next;

        list.insert (std::string(current->de_entry_name));

        disir_entry_finished (&current);
        current = next;
    }

    return (0);
}

int
Command::setup_group (std::string id)
{
    int code;
    std::list<std::string> groups;

    m_cli->list_groups (groups);

    if (std::find (groups.begin(), groups.end(), id) == groups.end())
    {
        // invalid entry
        std::cerr << "error: invalid group selected - " << id << std::endl;
        std::cerr << "Select one of the following:" << std::endl;
        for (auto& g : groups)
        {
            std::cerr << "  " << g << std::endl;
        }

        code = 1;
    }
    else
    {
        m_cli->group_id (id);
        m_cli->verbose() << "Setting user-provided group id to: "
                         << m_cli->group_id() << std::endl;
        code = 0;
    }

    return (code);
}

void
Command::print_verify (enum disir_status status, const char *entry,
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
