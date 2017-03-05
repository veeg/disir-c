#ifndef _LIBDISIRCLI_COMMAND_GENERATE_H
#define _LIBDISIRCLI_COMMAND_GENERATE_H

#include <set>

#include <disir/cli/cli.h>
#include <disir/cli/command.h>

namespace disir
{

    class CommandGenerate : public Command
    {
    public:
        //! Basic constructor
        CommandGenerate (void);

        //! Handle command implementation
        virtual int handle_command (std::vector<std::string> &args);

        // Generate a libdisir config file to specified filepath.
        int generate_libdisir (std::string &filepath);

        //! Generate all the entries passed in vector
        int generate_entries (std::set<std::string>& entries);

        // Query disir for available configs to generate that we have mold of, but not config.
        std::set<std::string> available_configs (void);

        //! Query disir for available namespaces.
        std::set<std::string> available_namespaces (void);

        //! List the input entries as available for generation
        int list_available_configs (std::set<std::string>& entries);

    private:
        bool m_force = false;
    };
}

#endif // _LIBDISIRCLI_COMMAND_GENERATE_H

