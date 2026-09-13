#include <nxdev/cli/commands.hpp>
#include <nxdev/device/device_manager.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace nxdev::cli {

int DevicesCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    std::string proj_root = (ctx.project.has_value() && ctx.project->is_valid()) ? ctx.project->root_path() : "";
    device::DeviceManager dm("", proj_root);

    if (args.empty() || args[0] == "--help" || args[0] == "-h") {
        std::cout << "Usage: nxdev devices <list|add|remove|show|set-default|test> [options]\n\n"
                  << "Manage target Nintendo Switch development devices.\n\n"
                  << "Subcommands:\n"
                  << "  list                     List all configured Switch devices (supports --json)\n"
                  << "  add <id> --host <ip>     Register a new device\n"
                  << "  remove <id>              Remove a configured device\n"
                  << "  show <id>                Show detailed information for a device\n"
                  << "  set-default <id>         Set a device as the default target\n"
                  << "  test <id|host>           Test network connectivity to a device\n";
        return 0;
    }

    if (args[0] == "list") {
        bool json_output = false;
        for (const auto& a : args) {
            if (a == "--json") json_output = true;
        }

        auto devices = dm.list_devices();

        if (json_output) {
            std::ostringstream oss;
            oss << "{\"devices\":[";
            for (size_t i = 0; i < devices.size(); ++i) {
                const auto& d = devices[i];
                if (i > 0) oss << ",";
                oss << "{\"id\":\"" << d.id << "\""
                    << ",\"name\":\"" << d.name << "\""
                    << ",\"host\":\"" << d.host << "\""
                    << ",\"transport\":\"" << d.transport << "\""
                    << ",\"port\":" << d.port
                    << ",\"default\":" << (d.is_default ? "true" : "false")
                    << ",\"created_at\":\"" << d.created_at << "\"}";
            }
            oss << "]}";
            std::cout << oss.str() << "\n";
            return 0;
        }

        std::cout << "Configured Switch Devices (" << dm.config_path() << "):\n\n";
        if (devices.empty()) {
            std::cout << "  (No devices registered. Use 'nxdev devices add <id> --host <ip>' to register a device)\n";
            return 0;
        }

        std::cout << "  " << std::left << std::setw(18) << "ID"
                  << std::setw(22) << "HOST"
                  << std::setw(12) << "TRANSPORT"
                  << std::setw(10) << "DEFAULT"
                  << "NAME\n";
        std::cout << "  " << std::string(70, '-') << "\n";

        for (const auto& d : devices) {
            std::cout << "  " << std::left << std::setw(18) << d.id
                      << std::setw(22) << d.host
                      << std::setw(12) << d.transport
                      << std::setw(10) << (d.is_default ? "YES" : "-")
                      << d.name << "\n";
        }
        return 0;
    }

    std::string sub = args[0];

    if (sub == "add") {
        if (args.size() < 2) {
            std::cerr << "Error: 'devices add' requires device ID and --host <ip/hostname>.\n"
                      << "Usage: nxdev devices add <id> --host <ip> [--name <name>] [--port <port>] [--default]\n";
            return 1;
        }

        device::Device dev;
        dev.id = args[1];
        dev.name = dev.id;
        bool set_default = false;

        for (size_t i = 2; i < args.size(); ++i) {
            if (args[i] == "--host" && i + 1 < args.size()) {
                dev.host = args[++i];
            } else if (args[i] == "--name" && i + 1 < args.size()) {
                dev.name = args[++i];
            } else if (args[i] == "--port" && i + 1 < args.size()) {
                try {
                    dev.port = static_cast<uint16_t>(std::stoi(args[++i]));
                } catch (...) {}
            } else if (args[i] == "--default") {
                set_default = true;
                dev.is_default = true;
            }
        }

        if (dev.host.empty()) {
            std::cerr << "Error: Missing required --host <ip/hostname> argument.\n";
            return 1;
        }

        if (!device::Device::is_valid_id(dev.id)) {
            std::cerr << "Error: Invalid device ID '" << dev.id << "'. Use alphanumeric characters, dashes, and underscores.\n";
            return 1;
        }

        if (!device::Device::is_valid_host(dev.host)) {
            std::cerr << "Error: Invalid host address '" << dev.host << "'.\n";
            return 1;
        }

        if (dm.add_device(dev, set_default)) {
            std::cout << "✔ Device '" << dev.id << "' (" << dev.host << ") added successfully.\n";
            return 0;
        } else {
            std::cerr << "Error: Failed to save device configuration.\n";
            return 1;
        }
    }

    if (sub == "remove" || sub == "rm") {
        if (args.size() < 2) {
            std::cerr << "Error: 'devices remove' requires device ID.\n";
            return 1;
        }
        std::string id = args[1];
        if (dm.remove_device(id)) {
            std::cout << "✔ Device '" << id << "' removed successfully.\n";
            return 0;
        } else {
            std::cerr << "Error: Device '" << id << "' not found.\n";
            return 1;
        }
    }

    if (sub == "set-default") {
        if (args.size() < 2) {
            std::cerr << "Error: 'devices set-default' requires device ID.\n";
            return 1;
        }
        std::string id = args[1];
        if (dm.set_default_device(id)) {
            std::cout << "✔ Device '" << id << "' set as default.\n";
            return 0;
        } else {
            std::cerr << "Error: Device '" << id << "' not found.\n";
            return 1;
        }
    }

    if (sub == "show") {
        if (args.size() < 2) {
            std::cerr << "Error: 'devices show' requires device ID.\n";
            return 1;
        }
        std::string id = args[1];
        bool json_output = (args.size() > 2 && args[2] == "--json");

        auto dev = dm.get_device(id);
        if (!dev) {
            std::cerr << "Error: Device '" << id << "' not found.\n";
            return 1;
        }

        if (json_output) {
            std::cout << "{\"id\":\"" << dev->id << "\""
                      << ",\"name\":\"" << dev->name << "\""
                      << ",\"host\":\"" << dev->host << "\""
                      << ",\"transport\":\"" << dev->transport << "\""
                      << ",\"port\":" << dev->port
                      << ",\"default\":" << (dev->is_default ? "true" : "false")
                      << ",\"created_at\":\"" << dev->created_at << "\"}\n";
            return 0;
        }

        std::cout << "Device Details: " << dev->id << "\n";
        std::cout << "  Name:       " << dev->name << "\n";
        std::cout << "  Host:       " << dev->host << "\n";
        std::cout << "  Transport:  " << dev->transport << "\n";
        std::cout << "  Port:       " << (dev->port > 0 ? std::to_string(dev->port) : "default (28280)") << "\n";
        std::cout << "  Default:    " << (dev->is_default ? "Yes" : "No") << "\n";
        std::cout << "  Created At: " << dev->created_at << "\n";
        return 0;
    }

    if (sub == "test") {
        if (args.size() < 2) {
            std::cerr << "Error: 'devices test' requires device ID or host.\n";
            return 1;
        }
        std::string query = args[1];
        bool json_output = false;
        for (const auto& a : args) if (a == "--json") json_output = true;

        auto dev = dm.find_device(query);
        device::Device target_dev;
        if (dev) {
            target_dev = *dev;
        } else if (device::Device::is_valid_host(query)) {
            target_dev.id = "test-target";
            target_dev.host = query;
        } else {
            std::cerr << "Error: Device or host '" << query << "' not found.\n";
            return 1;
        }

        if (!json_output) {
            std::cout << "Testing connectivity to " << target_dev.host << "...\n";
        }

        bool ok = device::DeviceManager::test_connectivity(target_dev, 3000);

        if (json_output) {
            std::cout << "{\"device\":\"" << target_dev.id << "\",\"host\":\"" << target_dev.host << "\",\"reachable\":" << (ok ? "true" : "false") << "}\n";
        } else {
            if (ok) {
                std::cout << "✔ Connection successful (Host " << target_dev.host << " is reachable)\n";
            } else {
                std::cout << "✗ Connection timed out or refused (Host " << target_dev.host << " is not responding)\n"
                          << "  Hint: Ensure the Switch is powered on, connected to the same Wi-Fi network, and running Homebrew Menu (nxlink server mode).\n";
            }
        }
        return ok ? 0 : 1;
    }

    std::cerr << "Unknown devices subcommand: '" << sub << "'. Available: list, add, remove, set-default, show, test.\n";
    return 1;
}

} // namespace nxdev::cli
