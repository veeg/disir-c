#ifndef _LIBDISIRCLI_COMMAND_H
#define _LIBDISIRCLI_COMMAND_H

#include <string>
#include <memory>
#include <vector>
#include <set>

#include <disir/disir.h>
#include <disir/cli/args.hxx>

namespace disir
{
    // Forward declare
    class Cli;

    //! Abstract baseclass used to implement CLI commands
    class Command
    {
        // Declare the CLI afriend class - so that it can access members
        friend class Cli;

    protected:
        //! Constructor - only subclasses can invoke
        Command (std::string name);

        virtual ~Command () = default;

        //! This will be the function invoked to handle a particular command
        virtual int handle_command (std::vector<std::string> &args) = 0;

        //! Return a set of available configs on the system
        int list_configs (std::set<std::string>& list);

        int setup_group (std::string);

        void setup_parser (args::ArgumentParser& parser);

        // Print the unserialized config or mold (not both) and
        // verify its content if the input status is not ok.
        void print_verify (enum disir_status status, const char *entry,
                           struct disir_config *config, struct disir_mold *mold);

    // Variables
    protected:
        //! Instance of the CLI object owning everything.
        std::shared_ptr<Cli>    m_cli;

        //! Name of the command
        const std::string       m_name;
    };
}

#endif // _LIBDISIRCLI_COMMAND_H

