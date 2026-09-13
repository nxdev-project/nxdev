#include <nxdev/device/device.hpp>
#include <nxdev/device/device_manager.hpp>
#include <cassert>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

void test_device_validation() {
    std::cout << "  ✓ test_device_validation..." << std::flush;

    // Valid IDs
    assert(nxdev::device::Device::is_valid_id("switch-1"));
    assert(nxdev::device::Device::is_valid_id("dev_switch"));
    assert(nxdev::device::Device::is_valid_id("mySwitch2026"));
    assert(nxdev::device::Device::is_valid_id("A"));

    // Invalid IDs
    assert(!nxdev::device::Device::is_valid_id(""));
    assert(!nxdev::device::Device::is_valid_id("-switch")); // Starts with dash
    assert(!nxdev::device::Device::is_valid_id("switch with space"));
    assert(!nxdev::device::Device::is_valid_id("switch/slash"));
    assert(!nxdev::device::Device::is_valid_id("switch;injection"));

    // Valid Hosts
    assert(nxdev::device::Device::is_valid_host("192.168.1.50"));
    assert(nxdev::device::Device::is_valid_host("10.0.0.1"));
    assert(nxdev::device::Device::is_valid_host("switch.local"));
    assert(nxdev::device::Device::is_valid_host("my-switch"));
    assert(nxdev::device::Device::is_valid_host("fe80::1"));
    assert(nxdev::device::Device::is_valid_host("[::1]"));

    // Invalid Hosts
    assert(!nxdev::device::Device::is_valid_host(""));
    assert(!nxdev::device::Device::is_valid_host("256.300.1.1"));
    assert(!nxdev::device::Device::is_valid_host("host with spaces"));
    assert(!nxdev::device::Device::is_valid_host("host;rm -rf"));

    std::cout << " passed\n";
}

void test_device_manager_crud() {
    std::cout << "  ✓ test_device_manager_crud..." << std::flush;

    fs::path temp_config = fs::temp_directory_path() / "nxdev_test_devices.yaml";
    if (fs::exists(temp_config)) fs::remove(temp_config);

    {
        nxdev::device::DeviceManager dm(temp_config.string());
        assert(dm.list_devices().empty());

        nxdev::device::Device d1;
        d1.id = "switch-dev";
        d1.name = "Development Switch";
        d1.host = "192.168.1.100";
        d1.transport = "nxlink";
        d1.port = 28280;

        assert(dm.add_device(d1, true));
        assert(dm.list_devices().size() == 1);
        assert(dm.get_default_device().has_value());
        assert(dm.get_default_device()->id == "switch-dev");

        nxdev::device::Device d2;
        d2.id = "switch-livingroom";
        d2.name = "Living Room Switch";
        d2.host = "192.168.1.150";

        assert(dm.add_device(d2, false));
        assert(dm.list_devices().size() == 2);
        assert(dm.get_default_device()->id == "switch-dev");

        // Switch default
        assert(dm.set_default_device("switch-livingroom"));
        assert(dm.get_default_device()->id == "switch-livingroom");

        // Remove device
        assert(dm.remove_device("switch-dev"));
        assert(dm.list_devices().size() == 1);
        assert(dm.get_default_device()->id == "switch-livingroom");
    }

    // Verify persistence across reload
    {
        nxdev::device::DeviceManager dm(temp_config.string());
        assert(dm.list_devices().size() == 1);
        auto dev = dm.get_device("switch-livingroom");
        assert(dev.has_value());
        assert(dev->host == "192.168.1.150");
        assert(dev->is_default);
    }

    if (fs::exists(temp_config)) fs::remove(temp_config);
    std::cout << " passed\n";
}

void test_device_precedence_resolution() {
    std::cout << "  ✓ test_device_precedence_resolution..." << std::flush;

    fs::path temp_config = fs::temp_directory_path() / "nxdev_test_prec.yaml";
    if (fs::exists(temp_config)) fs::remove(temp_config);

    nxdev::device::DeviceManager dm(temp_config.string());
    nxdev::device::Device d1;
    d1.id = "dev1";
    d1.name = "Dev 1";
    d1.host = "192.168.1.10";
    d1.is_default = true;

    nxdev::device::Device d2;
    d2.id = "dev2";
    d2.name = "Dev 2";
    d2.host = "192.168.1.20";
    d2.is_default = false;

    dm.add_device(d1, true);
    dm.add_device(d2, false);

    nxdev::device::Device out_dev;
    std::string err;

    // 1. Explicit host takes highest precedence
    assert(dm.resolve_target("10.0.0.99", "dev2", out_dev, err));
    assert(out_dev.host == "10.0.0.99");

    // 2. Explicit device takes precedence over default
    assert(dm.resolve_target("", "dev2", out_dev, err));
    assert(out_dev.id == "dev2");
    assert(out_dev.host == "192.168.1.20");

    // 3. Fallback to default device
    assert(dm.resolve_target("", "", out_dev, err));
    assert(out_dev.id == "dev1");
    assert(out_dev.host == "192.168.1.10");

    if (fs::exists(temp_config)) fs::remove(temp_config);
    std::cout << " passed\n";
}

int main() {
    std::cout << "[Test] Running Device Subsystem Tests...\n";
    test_device_validation();
    test_device_manager_crud();
    test_device_precedence_resolution();
    std::cout << "[Test] All Device Subsystem tests passed!\n";
    return 0;
}
