# NXDev Tests CMakeLists.txt

add_executable(test_version unit/test_version.cpp)
target_link_libraries(test_version PRIVATE NXDev::Core)
nxdev_configure_target(test_version)
add_test(NAME VersionTest COMMAND test_version)
set_tests_properties(VersionTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_sdk_core unit/test_sdk_core.cpp)
target_link_libraries(test_sdk_core PRIVATE NXDev::Core)
nxdev_configure_target(test_sdk_core)
add_test(NAME SdkCoreTest COMMAND test_sdk_core)
set_tests_properties(SdkCoreTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_modules unit/test_modules.cpp)
target_link_libraries(test_modules PRIVATE
    NXDev::Core
    NXDev::Input
    NXDev::Filesystem
    NXDev::Account
    NXDev::Network
    NXDev::Time
    NXDev::Power
)
nxdev_configure_target(test_modules)
add_test(NAME SdkModulesTest COMMAND test_modules)
set_tests_properties(SdkModulesTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_ui unit/test_ui.cpp)
target_link_libraries(test_ui PRIVATE
    NXDev::Core
    NXDev::Borealis
)
nxdev_configure_target(test_ui)
add_test(NAME SdkUiTest COMMAND test_ui)
set_tests_properties(SdkUiTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")


add_executable(test_manifest unit/test_manifest.cpp)
target_link_libraries(test_manifest PRIVATE NXDevAppManifest::Parser)
target_compile_definitions(test_manifest PRIVATE NXDEV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
nxdev_configure_target(test_manifest)
add_test(NAME ManifestParserTest COMMAND test_manifest)
set_tests_properties(ManifestParserTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_pack unit/test_pack.cpp)
target_link_libraries(test_pack PRIVATE NXDevPack::Core)
nxdev_configure_target(test_pack)
add_test(NAME PackManagerTest COMMAND test_pack)
set_tests_properties(PackManagerTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_environment unit/test_environment.cpp)
target_link_libraries(test_environment PRIVATE NXDev::CLI)
target_compile_definitions(test_environment PRIVATE NXDEV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
nxdev_configure_target(test_environment)
add_test(NAME EnvironmentTest COMMAND test_environment)
set_tests_properties(EnvironmentTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_build unit/test_build.cpp)
target_link_libraries(test_build PRIVATE NXDev::CLI)
target_compile_definitions(test_build PRIVATE NXDEV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
nxdev_configure_target(test_build)
add_test(NAME BuildPipelineTest COMMAND test_build)
set_tests_properties(BuildPipelineTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_packages unit/test_packages.cpp)
target_link_libraries(test_packages PRIVATE NXDev::CLI)
target_compile_definitions(test_packages PRIVATE NXDEV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
nxdev_configure_target(test_packages)
add_test(NAME PackageSubsystemTest COMMAND test_packages)
set_tests_properties(PackageSubsystemTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_resource_control unit/test_resource_control.cpp)
target_link_libraries(test_resource_control PRIVATE NXDev::CLI)
target_compile_definitions(test_resource_control PRIVATE NXDEV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
nxdev_configure_target(test_resource_control)
add_test(NAME ResourceControlTest COMMAND test_resource_control)
set_tests_properties(ResourceControlTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_device test_device.cpp)
target_link_libraries(test_device PRIVATE NXDev::CLI)
target_compile_definitions(test_device PRIVATE NXDEV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
nxdev_configure_target(test_device)
add_test(NAME DeviceTest COMMAND test_device)
set_tests_properties(DeviceTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_symbolizer test_symbolizer.cpp)
target_link_libraries(test_symbolizer PRIVATE NXDev::CLI)
target_compile_definitions(test_symbolizer PRIVATE NXDEV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
nxdev_configure_target(test_symbolizer)
add_test(NAME SymbolizerTest COMMAND test_symbolizer)
set_tests_properties(SymbolizerTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_run test_run.cpp)
target_link_libraries(test_run PRIVATE NXDev::CLI)
target_compile_definitions(test_run PRIVATE NXDEV_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
nxdev_configure_target(test_run)
add_test(NAME RunSubsystemTest COMMAND test_run)
set_tests_properties(RunSubsystemTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_init test_init.cpp)
target_link_libraries(test_init PRIVATE NXDev::CLI NXDevAppManifest::Parser)
nxdev_configure_target(test_init)
add_test(NAME InitSubsystemTest COMMAND test_init)
set_tests_properties(InitSubsystemTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_prepare test_prepare.cpp)
target_link_libraries(test_prepare PRIVATE NXDev::CLI)
nxdev_configure_target(test_prepare)
add_test(NAME PrepareSubsystemTest COMMAND test_prepare)
set_tests_properties(PrepareSubsystemTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_sdk_package test_sdk_package.cpp)
target_link_libraries(test_sdk_package PRIVATE NXDev::CLI)
nxdev_configure_target(test_sdk_package)
add_test(NAME SdkPackagingTest COMMAND test_sdk_package)
set_tests_properties(SdkPackagingTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

add_executable(test_header_leak test_header_leak.cpp)
nxdev_configure_target(test_header_leak)
add_test(NAME HeaderIsolationTest COMMAND test_header_leak)
set_tests_properties(HeaderIsolationTest PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")


if(TARGET nxdev)
    add_test(NAME CliHelpTest COMMAND nxdev --help)
    add_test(NAME CliVersionTest COMMAND nxdev --version)
    add_test(NAME CliEnvTest COMMAND nxdev env)
    add_test(NAME CliEnvJsonTest COMMAND nxdev env --json)
    add_test(NAME CliDoctorJsonTest COMMAND nxdev doctor --json)
    add_test(NAME CliDoctorBuildTest COMMAND nxdev doctor --build --json)
    add_test(NAME CliDoctorRunTest COMMAND nxdev doctor --run --json)
    add_test(NAME CliProjectInfoTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/manifests/standard" project info)
    add_test(NAME CliProjectRootTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/manifests/standard" project root)
    add_test(NAME CliConfigPathTest COMMAND nxdev config path)
    add_test(NAME CliManifestValidateMinimal COMMAND nxdev manifest validate "${CMAKE_SOURCE_DIR}/examples/manifests/minimal/nxapp.yaml")
    add_test(NAME CliManifestValidateStandard COMMAND nxdev manifest validate "${CMAKE_SOURCE_DIR}/examples/manifests/standard/nxapp.yaml")
    add_test(NAME CliManifestValidateAdvanced COMMAND nxdev manifest validate "${CMAKE_SOURCE_DIR}/examples/manifests/advanced/nxapp.yaml")
    add_test(NAME CliManifestInspectJson COMMAND nxdev manifest inspect "${CMAKE_SOURCE_DIR}/examples/manifests/standard/nxapp.yaml" --json)
    add_test(NAME CliManifestSchema COMMAND nxdev manifest schema)
    add_test(NAME CliPackageHelpTest COMMAND nxdev package --help)
    add_test(NAME CliPackageListTest COMMAND nxdev package list)
    add_test(NAME CliPackageListJsonTest COMMAND nxdev package list --json)
    add_test(NAME CliPackageInfoTest COMMAND nxdev package info nxdev.sdl2)
    add_test(NAME CliPackageInfoJsonTest COMMAND nxdev package info nxdev.sdl2 --json)
    add_test(NAME CliPackageStatusTest COMMAND nxdev package status)
    add_test(NAME CliPackageStatusJsonTest COMMAND nxdev package status --json)
    add_test(NAME CliPackageSearchTest COMMAND nxdev package search sdl)
    add_test(NAME CliPackageInstallDryRunTest COMMAND nxdev package install nxdev.sdl2 --dry-run)
    add_test(NAME CliPackageInstallMissingDryRunTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/sdl2-demo" package install --missing --dry-run)
    add_test(NAME CliConfigureTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/hello-switch" configure --json)
    add_test(NAME CliBuildTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/hello-switch" build --json)
    add_test(NAME CliCleanTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/hello-switch" clean)
    add_test(NAME CliConfigureNxDevTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/hello-nxdev" configure --json)
    add_test(NAME CliBuildNxDevTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/hello-nxdev" build --json)
    add_test(NAME CliCleanNxDevTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/hello-nxdev" clean)
    add_test(NAME CliConfigurePlatformDemoTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/platform-demo" configure --json)
    add_test(NAME CliBuildPlatformDemoTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/platform-demo" build --json)
    add_test(NAME CliCleanPlatformDemoTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/platform-demo" clean)
    add_test(NAME CliConfigureSdl2DemoTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/sdl2-demo" configure --json)
    add_test(NAME CliBuildSdl2DemoTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/sdl2-demo" build --json)
    add_test(NAME CliCleanSdl2DemoTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/sdl2-demo" clean)
    add_test(NAME CliPackHelpTest COMMAND nxdev pack --help)
    add_test(NAME CliPackNroHelpTest COMMAND nxdev pack nro --help)
    add_test(NAME CliPackNspHelpTest COMMAND nxdev pack nsp --help)
    add_test(NAME CliPackNroDryRunTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/hello-switch" pack nro --dry-run)
    add_test(NAME CliPackNroDemoBuildTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/nro-demo" build --json)
    add_test(NAME CliPackNroDemoPackTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/nro-demo" pack nro --json)
    add_test(NAME CliPackNroDemoNoBuildTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/nro-demo" pack nro --no-build --json)
    add_test(NAME CliPackNspDryRunTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/nsp-demo" pack nsp --dry-run)
    add_test(NAME CliPackNspDemoBuildTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/nsp-demo" build --json)
    add_test(NAME CliPackNspDemoPackTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/nsp-demo" pack nsp --keys "${CMAKE_SOURCE_DIR}/tests/fixtures/dummy.keys" --json)
    add_test(NAME CliPackNspDemoNoBuildTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/nsp-demo" pack nsp --keys "${CMAKE_SOURCE_DIR}/tests/fixtures/dummy.keys" --no-build --json)
    add_test(NAME CliDevicesHelpTest COMMAND nxdev devices --help)
    add_test(NAME CliDevicesListTest COMMAND nxdev devices list)
    add_test(NAME CliDevicesListJsonTest COMMAND nxdev devices list --json)
    add_test(NAME CliDevicesAddTest COMMAND nxdev devices add ci-switch --host 192.168.1.50 --name "CI Switch" --default)
    add_test(NAME CliDevicesShowJsonTest COMMAND nxdev devices show ci-switch --json)
    add_test(NAME CliDevicesSetDefaultTest COMMAND nxdev devices set-default ci-switch)
    add_test(NAME CliDevicesRemoveTest COMMAND nxdev devices remove ci-switch)
    add_test(NAME CliDeployDryRunTest COMMAND nxdev deploy --artifact "${CMAKE_SOURCE_DIR}/examples/nro-demo/dist/debug/nro-demo.nro" --host 192.168.1.50 --dry-run --json)
    add_test(NAME CliRunDryRunTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/examples/nro-demo" run --host 192.168.1.50 --dry-run --json)
    add_test(NAME CliSymbolizeTest COMMAND nxdev symbolize --elf "${CMAKE_SOURCE_DIR}/examples/nro-demo/.nxdev/build/debug/bin/nro-demo.elf" 0x7100001040 --json)
    add_test(NAME CliLogsListTest COMMAND nxdev logs list --json)
    add_test(NAME CliNewHelpTest COMMAND nxdev new --help)
    add_test(NAME CliNewListTest COMMAND nxdev new --list)
    add_test(NAME CliNewListJsonTest COMMAND nxdev new --list --json)
    add_test(NAME CliNewScaffoldTest COMMAND nxdev new "Smoke Generated App" --dir "${CMAKE_BINARY_DIR}/tmp_new_app" --template console-cpp --force --json)
    add_test(NAME CliNewValidateManifestTest COMMAND nxdev manifest validate "${CMAKE_BINARY_DIR}/tmp_new_app/nxapp.yaml")
    add_test(NAME CliConfigureSmokeTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/tests/projects/smoke" configure --json)
    add_test(NAME CliBuildSmokeTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/tests/projects/smoke" build --json)
    add_test(NAME CliPackNroSmokeTest COMMAND nxdev --project "${CMAKE_SOURCE_DIR}/tests/projects/smoke" pack nro --json --dry-run)
    add_test(NAME CliInitHelpTest COMMAND nxdev init --help)
    add_test(NAME CliPrepareHelpTest COMMAND nxdev prepare --help)
    add_test(NAME CliPrepareCheckTest COMMAND nxdev prepare --check --json)
    add_test(NAME CliSdkHelpTest COMMAND nxdev sdk --help)
    add_test(NAME CliSdkInfoJsonTest COMMAND nxdev sdk info --json)

    set_tests_properties(
        CliHelpTest CliVersionTest CliEnvTest CliEnvJsonTest CliDoctorJsonTest CliDoctorBuildTest CliDoctorRunTest
        CliProjectInfoTest CliProjectRootTest CliConfigPathTest
        CliManifestValidateMinimal CliManifestValidateStandard CliManifestValidateAdvanced
        CliManifestInspectJson CliManifestSchema
        CliPackageHelpTest CliPackageListTest CliPackageListJsonTest CliPackageInfoTest CliPackageInfoJsonTest
        CliPackageStatusTest CliPackageStatusJsonTest CliPackageSearchTest CliPackageInstallDryRunTest CliPackageInstallMissingDryRunTest
        CliConfigureTest CliBuildTest CliCleanTest
        CliConfigureNxDevTest CliBuildNxDevTest CliCleanNxDevTest
        CliConfigurePlatformDemoTest CliBuildPlatformDemoTest CliCleanPlatformDemoTest
        CliConfigureSdl2DemoTest CliBuildSdl2DemoTest CliCleanSdl2DemoTest
        CliPackHelpTest CliPackNroHelpTest CliPackNspHelpTest CliPackNroDryRunTest
        CliPackNroDemoBuildTest CliPackNroDemoPackTest CliPackNroDemoNoBuildTest
        CliPackNspDryRunTest CliPackNspDemoBuildTest CliPackNspDemoPackTest CliPackNspDemoNoBuildTest
        CliDevicesHelpTest CliDevicesListTest CliDevicesListJsonTest CliDevicesAddTest CliDevicesShowJsonTest
        CliDevicesSetDefaultTest CliDevicesRemoveTest CliDeployDryRunTest CliRunDryRunTest CliSymbolizeTest CliLogsListTest
        CliNewHelpTest CliNewListTest CliNewListJsonTest CliNewScaffoldTest CliNewValidateManifestTest
        CliConfigureSmokeTest CliBuildSmokeTest CliPackNroSmokeTest
        CliInitHelpTest CliPrepareHelpTest CliPrepareCheckTest CliSdkHelpTest CliSdkInfoJsonTest
        PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
endif()

