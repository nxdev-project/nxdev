#include <nxdev/run/deploy_service.hpp>
#include <nxdev/run/run_session.hpp>
#include <cassert>
#include <iostream>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void test_deploy_service_dry_run() {
    std::cout << "  ✓ test_deploy_service_dry_run..." << std::flush;

    // Create a temporary dummy NRO
    fs::path temp_nro = fs::temp_directory_path() / "nxdev_test_app.nro";
    std::ofstream out(temp_nro);
    out << "NRO0 dummy content";
    out.close();

    nxdev::run::DeployOptions opts;
    opts.device.id = "test-switch";
    opts.device.host = "192.168.1.50";
    opts.artifact_path = temp_nro.string();
    opts.dry_run = true;

    nxdev::run::DeployService svc;
    auto res = svc.deploy(opts);

    assert(res.success);
    assert(res.device_id == "test-switch");
    assert(res.device_host == "192.168.1.50");

    if (fs::exists(temp_nro)) fs::remove(temp_nro);
    std::cout << " passed\n";
}

void test_run_session_dry_run() {
    std::cout << "  ✓ test_run_session_dry_run..." << std::flush;

    nxdev::run::RunOptions opts;
    opts.device.id = "test-switch";
    opts.device.host = "192.168.1.50";
    opts.dry_run = true;
    opts.json_stream = false;

    nxdev::run::RunSession session(opts);
    auto summary = session.execute();

    assert(summary.success);
    assert(summary.device_id == "test-switch");
    assert(summary.final_state == nxdev::run::SessionState::Exited);

    std::cout << " passed\n";
}

int main() {
    std::cout << "[Test] Running Run & Deploy Subsystem Tests...\n";
    test_deploy_service_dry_run();
    test_run_session_dry_run();
    std::cout << "[Test] All Run & Deploy Subsystem tests passed!\n";
    return 0;
}
