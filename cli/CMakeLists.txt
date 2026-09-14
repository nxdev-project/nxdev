# NXDev CLI CMakeLists.txt

add_library(nxdev_cli_lib STATIC
    src/exec/resource_calculator.cpp
    src/exec/controlled_runner.cpp
    src/exec/process.cpp
    src/env/host_info.cpp
    src/env/toolchain_info.cpp
    src/env/environment.cpp
    src/config/host_config.cpp
    src/project/project.cpp
    src/doctor/doctor.cpp
    src/packages/package_definition.cpp
    src/packages/registry.cpp
    src/packages/resolver.cpp
    src/packages/package_manager.cpp
    src/build/build_orchestrator.cpp
    src/device/device.cpp
    src/device/device_manager.cpp
    src/symbolize/symbolizer.cpp
    src/run/deploy_service.cpp
    src/run/run_session.cpp
    src/cli.cpp
    src/commands/version_cmd.cpp
    src/commands/env_cmd.cpp
    src/commands/doctor_cmd.cpp
    src/commands/project_cmd.cpp
    src/commands/config_cmd.cpp
    src/commands/manifest_cmd.cpp
    src/commands/package_cmd.cpp
    src/commands/configure_cmd.cpp
    src/commands/build_cmd.cpp
    src/commands/clean_cmd.cpp
    src/commands/pack_cmd.cpp
    src/commands/new_cmd.cpp
    src/commands/devices_cmd.cpp
    src/commands/run_cmd.cpp
    src/commands/deploy_cmd.cpp
    src/commands/symbolize_cmd.cpp
    src/commands/logs_cmd.cpp
    src/commands/init_cmd.cpp
    src/commands/prepare_cmd.cpp
    src/commands/sdk_cmd.cpp
)

add_library(NXDev::CLI ALIAS nxdev_cli_lib)

target_include_directories(nxdev_cli_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(nxdev_cli_lib PUBLIC
    NXDev::Core
    NXDevAppManifest::Parser
    NXDevPack::Core
)

target_compile_definitions(nxdev_cli_lib PUBLIC NXDEV_VERSION="${NXDEV_VERSION_STRING}")

nxdev_configure_target(nxdev_cli_lib)

add_executable(nxdev src/main.cpp)
target_link_libraries(nxdev PRIVATE NXDev::CLI)
nxdev_configure_target(nxdev)

# Install rule for CLI
install(TARGETS nxdev DESTINATION bin)
