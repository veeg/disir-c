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

    };

}

#endif // _LIBDISIRCLI_COMMAND_VERIFY_H

