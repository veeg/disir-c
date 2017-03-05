#ifndef _LIBDISIRCLI_CLI_H
#define _LIBDISIRCLI_CLI_H

#include <memory>
#include <map>
#include <ostream>

#include <disir/disir.h>

#include <disir/cli/command.h>

namespace disir
{
    class Cli : public std::enable_shared_from_this<Cli>
    {
    public:
        // Basic constructor
        Cli (const std::string &program_name);

        // Print the version information for disir to ostringstream
        void version (std::ostringstream &os);

        // Print the available commands list to ostringstream
        void available_commands (std::ostringstream &os);

        // Populate the CLI object with all default commands
        // Default commands must be explicitly added after class construction
        void add_default_commands (void);

        //! Add a command to the CLI
        void add_command (std::shared_ptr<Command> command);

        //! Return the allocated disir instance pointer
        // XXX: Is this a clash?
        struct disir_instance * disir(void);

        //! Run the cli with the given argument
        //!
        //! \return Status code that may be used as process exit status.
        int run (std::vector<std::string> &args);

        std::string& group_id (void);

        std::string& group_id (std::string& id);

        std::ostream& verbose (void);

    private:
        // Actually marge the command line arguments
        // This will populate several CLI members
        int parse_command_line_args (std::vector<std::string> &args);

        // Initialize the libdisir library with m_config_filepath configuration
        int initialize_disir (void);

        //! Output no matching command message.
        //! Performs fuzzy matching against other registered commands.
        void no_matching_command (void);

        //! Fuzzy match command name with register non-hidden commands
        //! if any match is found, the output stream is populated with
        //! an appropriate error message and listing. Function returns true
        //! if no match is found, nothing is emitted to the output stream
        //! and false is returned.
        const bool report_related (const std::string& command_name, std::ostringstream& os);

    public:
        //! Program name
        const std::string m_program_name;


    private:
        //! Config filepath
        std::string m_config_filepath = "";

        //! The active command for this cli instance, if applicable.
        std::string m_active_command = "";

        //! The group_id to operate on
        std::string m_group_id = "test";

        //! Map of commands available
        std::map<std::string, std::shared_ptr<Command>> m_commands;

        bool m_handle_version = false;
        bool m_handle_help = false;
        bool m_verbose = false;

        //! This is a zero allocated ostream - its basically acts as the /dev/null of ostreams
        //! This is used as return value for verbose() if verbose is not enabled.
        std::ostream   m_ostream_sink;

        //! Help text output from the parser
        std::string m_help_text = "";

        //! Internal disr instance pointer.
        struct disir_instance *m_disir;
    };
}

#endif // _LIBDISIRCLI_CLI_H

