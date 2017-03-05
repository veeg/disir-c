#ifndef _LIBDISIRCLI_COMMAND_LIST_H
#define _LIBDISIRCLI_COMMAND_LIST_H

#include <string>

#include <disir/cli/cli.h>
#include <disir/cli/command.h>

namespace disir
{
    class CommandList : public Command
    {
    public:
        //! Basic constructor
        CommandList (void);

        //! Handle command implementation
        virtual int handle_command (std::vector<std::string> &args);

    private:
        int list_input_plugin(void);
        int list_config (std::string &type);
    };

}

#endif // _LIBDISIRCLI_COMMAND_LIST_H

