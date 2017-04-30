#ifndef _LIBDISIRCLI_COMMAND_VERIFY_H
#define _LIBDISIRCLI_COMMAND_VERIFY_H

#include <string>

#include <disir/cli/cli.h>
#include <disir/cli/command.h>

namespace disir
{
    class CommandVerify : public Command
    {
    public:
        //! Basic constructor
        CommandVerify (void);

        //! Handle command implementation
        virtual int handle_command (std::vector<std::string> &args);

        // Print the unserialized config or mold (not both) and
        // verify its content if the input status is not ok.
        void print_verify (enum disir_status status, const char *entry,
                           struct disir_config *config, struct disir_mold *mold);
    };

}

#endif // _LIBDISIRCLI_COMMAND_VERIFY_H

