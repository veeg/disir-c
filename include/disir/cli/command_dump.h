#ifndef _LIBDISIRCLI_COMMAND_DUMP_H
#define _LIBDISIRCLI_COMMAND_DUMP_H

#include <string>

#include <disir/cli/cli.h>
#include <disir/cli/command.h>

namespace disir
{
    class CommandDump : public Command
    {
    public:
        //! Basic constructor
        CommandDump (void);

        //! Handle command implementation
        virtual int handle_command (std::vector<std::string> &args);

        int output_config_context (struct disir_context *context, int indent);
    };

}

#endif // _LIBDISIRCLI_COMMAND_DUMP_H

