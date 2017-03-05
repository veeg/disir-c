#include <vector>

#include <disir/cli/cli.h>

int
main (int argc, char *argv[])
{
    std::shared_ptr<disir::Cli> cli;
    std::vector<std::string> args (argv + 1, argv + argc);

    cli = std::make_shared<disir::Cli> ("disir");
    cli->add_default_commands ();

    return cli->run (args);
}

