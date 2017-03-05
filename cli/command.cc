#include <disir/cli/command.h>
#include <disir/cli/cli.h>

#include <iostream>

using namespace disir;

Command::Command (std::string name)
    : m_name (name)
{
}

std::set<std::string>
Command::list_configs (void)
{
    enum disir_status status;
    struct disir_entry *config_entries;
    struct disir_entry *next;
    struct disir_entry *current;
    std::set<std::string> list;

    status = disir_config_entries (m_cli->disir(), m_cli->group_id().c_str(), &config_entries);
    if (status != DISIR_STATUS_OK)
    {
        std::cerr << "Failed to retrieve available configs: "
                  << disir_error (m_cli->disir()) << std::endl;
        return list;
    }

    current = config_entries;
    while (current != NULL)
    {
        next = current->next;

        list.insert (std::string(current->de_entry_name));

        disir_entry_finished (&current);
        current = next;
    }

    return list;
}

