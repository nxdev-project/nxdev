#include <nxdev/cli/cli.hpp>

int main(int argc, char* argv[]) {
    nxdev::cli::CLI app;
    return app.run(argc, argv);
}
