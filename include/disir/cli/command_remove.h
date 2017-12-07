#ifndef _LIBDISIRCLI_COMMAND_REMOVE_H
#define _LIBDISIRCLI_COMMAND_REMOVE_H

#include <string>

#include <disir/cli/cli.h>
#include <disir/cli/command.h>

namespace disir
{
    class CommandRemove : public Command
    {
    public:
        //! Basic constructor
        CommandRemove (void);

        //! Handle command implementation
        virtual int handle_command (std::vector<std::string> &args);
    };

}

#endif // _LIBDISIRCLI_COMMAND_REMOVE_H

