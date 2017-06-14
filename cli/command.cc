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
        m_cli->verbose() << "Setting user-provided group id to: " << m_cli->group_id << std::endl;
        code = 0;
    }

    return (code);
}
