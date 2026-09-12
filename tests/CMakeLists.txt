# NXDev Tests CMakeLists.txt

add_executable(test_version unit/test_version.cpp)
target_link_libraries(test_version PRIVATE NXDev::Core)
nxdev_configure_target(test_version)
add_test(NAME VersionTest COMMAND test_version)

add_executable(test_sdk_core unit/test_sdk_core.cpp)
target_link_libraries(test_sdk_core PRIVATE NXDev::Core NXDev::Input NXDev::Display)
nxdev_configure_target(test_sdk_core)
add_test(NAME SdkCoreTest COMMAND test_sdk_core)

add_executable(test_manifest unit/test_manifest.cpp)
target_link_libraries(test_manifest PRIVATE NXDevAppManifest::Parser)
nxdev_configure_target(test_manifest)
add_test(NAME ManifestParserTest COMMAND test_manifest)

add_executable(test_pack unit/test_pack.cpp)
target_link_libraries(test_pack PRIVATE NXDevPack::Core)
nxdev_configure_target(test_pack)
add_test(NAME PackManagerTest COMMAND test_pack)

if(TARGET nxdev)
    add_test(NAME CliHelpTest COMMAND nxdev --help)
    add_test(NAME CliVersionTest COMMAND nxdev --version)
    add_test(NAME CliDoctorTest COMMAND nxdev doctor)
    add_test(NAME CliManifestValidateMinimal COMMAND nxdev manifest validate examples/manifests/minimal/nxapp.yaml)
    add_test(NAME CliManifestValidateStandard COMMAND nxdev manifest validate examples/manifests/standard/nxapp.yaml)
    add_test(NAME CliManifestValidateAdvanced COMMAND nxdev manifest validate examples/manifests/advanced/nxapp.yaml)
    add_test(NAME CliManifestInspectJson COMMAND nxdev manifest inspect examples/manifests/standard/nxapp.yaml --json)
    add_test(NAME CliManifestSchema COMMAND nxdev manifest schema)
endif()
