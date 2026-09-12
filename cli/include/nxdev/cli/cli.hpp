#pragma once

#include <nxdev/cli/command.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nxdev::cli {

class CLI {
public:
    CLI();
    ~CLI() = default;

    void register_command(std::unique_ptr<ICommand> cmd);
    void print_help() const;
    void print_version() const;
    int run(int argc, char* argv[]);

private:
    std::map<std::string, std::unique_ptr<ICommand>> commands_;
};

} // namespace nxdev::cli
